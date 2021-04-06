/**
 * @name image_traversal.h
 * @brief functions providing indices for image traversal
 * @author Petr Flajšingr, xflajs00
 * @date 6.4.2021
 */

#ifndef HUFF_CODEC__IMAGE_TRAVERSAL_H
#define HUFF_CODEC__IMAGE_TRAVERSAL_H

#include "constants.h"
namespace pf::kko {

/**
 * State of finite automaton providing indices for traversal.
 */
enum class ZigZagState { Right, Down, LeftDown, RightUp };

/**
 * Finite automaton providing indices for traversal
 * @param coord current coordinates
 * @param state current state
 * @param width width of data
 * @param height height of data
 * @return new state and new coordinates
 */
inline std::pair<ZigZagState, Dimensions> zigZagMove(Dimensions coord, ZigZagState state, std::size_t width,
                                                     std::size_t height) {
  switch (state) {
    case ZigZagState::Right:
      ++coord.first;
      if (coord.second == height - 1) {
        return {ZigZagState::RightUp, coord};
      } else {
        return {ZigZagState::LeftDown, coord};
      }
    case ZigZagState::LeftDown:
      --coord.first;
      ++coord.second;
      if (coord.second == height - 1) {
        return {ZigZagState::Right, coord};
      } else if (coord.first == 0) {
        return {ZigZagState::Down, coord};
      }
      return {ZigZagState::LeftDown, coord};
    case ZigZagState::Down:
      ++coord.second;
      if (coord.first == width - 1) {
        return {ZigZagState::LeftDown, coord};
      } else {
        return {ZigZagState::RightUp, coord};
      }
    case ZigZagState::RightUp:
      ++coord.first;
      --coord.second;
      if (coord.first == width - 1) {
        return {ZigZagState::Down, coord};
      } else if (coord.second == 0) {
        return {ZigZagState::Right, coord};
      }
      return {ZigZagState::RightUp, coord};
  }
  throw std::runtime_error("Invalid state in zigZagMove");
}

/**
 * Provider of indices for horizontal traversal
 * @param index current index
 * @param blockDimensions size of block
 * @return
 */
inline Dimensions horizontal(std::size_t index, const Dimensions &blockDimensions) {
  const auto xPos = index % blockDimensions.first;
  const auto yPos = index / blockDimensions.first;
  return {xPos, yPos};
}

/**
 * Provider of indices for vertical traversal
 * @param index current index
 * @param blockDimensions size of block
 * @return
 */
inline Dimensions vertical(std::size_t index, const Dimensions &blockDimensions) {
  const auto yPos = index % blockDimensions.second;
  const auto xPos = index / blockDimensions.second;
  return {xPos, yPos};
}

/**
 * Provider of indices for Hilbert curve traversal
 * @param index current index
 * @param blockDimensions size of block
 * @return
 */
inline Dimensions hilbertCurve(std::size_t index, const Dimensions &blockDimensions) {
  constexpr auto positions = std::array<std::pair<std::size_t, std::size_t>, 4>{
      std::pair<std::size_t, std::size_t>{0, 0},
      {0, 1},
      {1, 1},
      {1, 0},
  };
  const auto position = positions[index & 3];
  index >>= 2;
  auto x = position.first;
  auto y = position.second;
  const auto N = blockDimensions.first * blockDimensions.second;
  for (std::size_t n = 4; n <= N; n *= 2) {
    const auto n2 = n / 2;
    switch (index & 3) {
      case 0: std::swap(x, y); break;
      case 1: y += n2; break;
      case 2:
        x += n2;
        y += n2;
        break;
      case 3:
        const auto tmp = y;
        y = n2 - 1 - x;
        x = n2 - 1 - tmp;
        x += n2;
        break;
    }
    index >>= 2;
  }

  return {x, y};
}

inline Dimensions mortonCurve(std::size_t index, const Dimensions &blockDimensions) {
  constexpr auto positions = std::array<std::pair<std::size_t, std::size_t>, 4>{
      std::pair<std::size_t, std::size_t>{0, 0},
      {1, 0},
      {0, 1},
      {1, 1},
  };
  auto x = index / 4 * 2 % blockDimensions.first;
  auto y = index / (2 * blockDimensions.second) * 2;
  const auto position = positions[index % 4];
  return {x + position.first, y + position.second};
}

}
#endif//HUFF_CODEC__IMAGE_TRAVERSAL_H

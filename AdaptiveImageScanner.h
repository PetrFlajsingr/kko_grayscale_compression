/**
 * @name AdaptiveImageScanner.h
 * @brief utilities for scanning 2D data through blocks
 * @author Petr Flajšingr, xflajs00
 * @date 31.03.2021
 */

#ifndef HUFF_CODEC__ADAPTIVEIMAGESCANNER_H
#define HUFF_CODEC__ADAPTIVEIMAGESCANNER_H

#include "block_scorers.h"
#include "image_traversal.h"
#include "View2D.h"
#include "constants.h"
#include "magic_enum.hpp"
#include "models.h"
#include <ranges>
#include <utility>

namespace pf::kko {

/**
 * Calculate start position of block within data.
 * @param index index of the block
 * @param imageWidth width of data
 * @param blockDimensions size of blocks
 * @return starting position of block within data
 */
Dimensions startPosForBlock(std::size_t index, std::size_t imageWidth, Dimensions blockDimensions) {
  const auto imageBlockWidth =
      static_cast<std::size_t>(std::ceil(imageWidth / static_cast<float>(blockDimensions.first)));
  const auto xBlockPos = index % imageBlockWidth;
  const auto yBlockPos = index / imageBlockWidth;
  return {xBlockPos * blockDimensions.first, yBlockPos * blockDimensions.second};
}
/**
 * Supported types of block traversal.
 */
enum class ScanMethod : uint8_t { Vertical = 0, Horizontal = 1, ZigZag = 2, HilbertCurve = 3, MortonCurve = 4 };

/**
 * Provider of Block objects, which represent blocks of data within supplied input data.
 * @tparam Scorer object for scoring block traversal suitability
 * @tparam NeighborModel data transforming object
 */
template<std::ranges::contiguous_range R, BlockScorer<uint8_t> Scorer, Model<uint8_t> NeighborModel>
class AdaptiveImageScanner {
 public:
  class Block;

  struct ImageBlockIteratorSentinel;
  class ImageBlockIterator;

  AdaptiveImageScanner(View2D<R, true> view, Dimensions blockSize, Scorer &&scorer, NeighborModel &&model)
      : imageView(view), blockDimensions(std::move(blockSize)), blockScorer(std::forward<Scorer>(scorer)),
        model(std::forward<NeighborModel>(model)) {}
  AdaptiveImageScanner(const R &data, std::size_t imgWidth, Dimensions blockSize, Scorer &&scorer,
                       NeighborModel &&model)
      : imageView(data, imgWidth), blockDimensions(std::move(blockSize)), blockScorer(std::forward<Scorer>(scorer)),
        model(std::forward<NeighborModel>(model)) {}

  [[nodiscard]] ImageBlockIterator begin() { return ImageBlockIterator{*this, 0}; }
  [[nodiscard]] ImageBlockIteratorSentinel end() { return ImageBlockIteratorSentinel{getBlockCount()}; }

  /**
   * @return amount of blocks within data
   */
  [[nodiscard]] std::size_t size() const { return getBlockCount(); }

  [[nodiscard]] const Dimensions &getBlockDimensions() const { return blockDimensions; }

 private:
  [[nodiscard]] std::size_t getBlockCount() const {
    const auto imageBlockWidth =
        static_cast<std::size_t>(std::ceil(imageView.getWidth() / static_cast<float>(blockDimensions.first)));
    const auto imageBlockHeight =
        static_cast<std::size_t>(std::ceil(imageView.size() / static_cast<float>(blockDimensions.second)));
    return imageBlockWidth * imageBlockHeight;
  }

  Block getBestBlockForIndex(std::size_t index) {
    auto result = Block(imageView, ScanMethod::Vertical, startPosForBlock(index, imageView.getWidth(), blockDimensions),
                        blockDimensions);
    auto bestScore = std::numeric_limits<int>::lowest();
    ScanMethod bestMethod = ScanMethod::Horizontal;
    for (auto scanMethod : magic_enum::enum_values<ScanMethod>()) {
      blockScorer.reset();
      result.setScanMethod(scanMethod);
      for (auto val : result) { blockScorer.next(val); }
      const auto score = blockScorer.getScore();
      if (score > bestScore) {
        bestScore = score;
        bestMethod = scanMethod;
        if (score == Scorer::MaxScore) { break; }
      }
    }
    result.setScanMethod(bestMethod);
    return result;
  }

  View2D<R, true> imageView;
  Dimensions blockDimensions;
  Scorer blockScorer;
  NeighborModel model;
};

/**
 * Represents selected block within data. Scans across the block with provided scan method.
 */
template<std::ranges::contiguous_range R, BlockScorer<uint8_t> Scorer, Model<uint8_t> NeighborModel>
class AdaptiveImageScanner<R, Scorer, NeighborModel>::Block {
 public:
  Block() = default;
  Block(const View2D<R, true> &imageView, ScanMethod scanMethod, Dimensions startPosition, Dimensions blockDimensions)
      : imageView(imageView), scanMethod(scanMethod), startPosition(std::move(startPosition)),
        blockDimensions(std::move(blockDimensions)) {}
  bool operator==(const Block &rhs) const { return scanMethod == rhs.scanMethod && startPosition == rhs.startPosition; }
  bool operator!=(const Block &rhs) const { return !(rhs == *this); }
  [[nodiscard]] ScanMethod getScanMethod() const { return scanMethod; }
  void setScanMethod(ScanMethod newScanMethod) { scanMethod = newScanMethod; }

  class Sentinel {
   public:
    Sentinel() = default;
    explicit Sentinel(size_t size) : size(size) {}
    [[nodiscard]] std::size_t getSize() const { return size; }

   private:
    std::size_t size{};
  };
  class Iterator {
   public:
    using value_type = uint8_t;
    using difference_type = int;
    using reference = const uint8_t &;
    Iterator() = default;
    explicit Iterator(std::observer_ptr<Block> block) : block(block), model(block->model) {}
    bool operator==(const Iterator &rhs) const { return block == rhs.block && currentIndex == rhs.currentIndex; }
    bool operator!=(const Iterator &rhs) const { return !(rhs == *this); }
    bool operator==(const Sentinel &other) const { return currentIndex == other.getSize(); }
    value_type operator*() const {
      Dimensions pos;
      if (block->getScanMethod() == ScanMethod::ZigZag) {
        pos = {block->startPosition.first + zigZagPos.first, block->startPosition.second + zigZagPos.second};
      } else {
        pos = block->getPosInView(currentIndex);
      }
      if (pos.first >= block->imageView.getWidth() || pos.second >= block->imageView.size()) { return 0; }
      return model.apply(block->imageView[pos.first][pos.second]);
    }
    Iterator &operator++() {
      if (block->getScanMethod() == ScanMethod::ZigZag) {
        const auto [newState, newPos] =
            zigZagMove(zigZagPos, zigZagState, block->blockDimensions.first, block->blockDimensions.second);
        zigZagState = newState;
        zigZagPos = newPos;
      }
      ++currentIndex;
      return *this;
    }
    Iterator operator++(int) {
      auto result = *this;
      operator++();
      return result;
    }

   private:
    std::observer_ptr<Block> block{};
    std::size_t currentIndex{};
    Dimensions zigZagPos{};
    ZigZagState zigZagState = ZigZagState::Right;
    mutable NeighborModel model;
  };

  [[nodiscard]] Iterator begin() { return Iterator{std::make_observer(this)}; }

  [[nodiscard]] Sentinel end() { return Sentinel(blockDimensions.first * blockDimensions.second); }

 private:
  [[nodiscard]] Dimensions getPosInView(std::size_t index) const {
    const auto posInBlock = getPosInBlock(index);
    return {startPosition.first + posInBlock.first, startPosition.second + posInBlock.second};
  }
  [[nodiscard]] Dimensions getPosInBlock(std::size_t index) const {
    switch (scanMethod) {
      case ScanMethod::Horizontal: return horizontal(index, blockDimensions);
      case ScanMethod::Vertical: return vertical(index, blockDimensions);
      case ScanMethod::ZigZag: throw std::runtime_error("Invalid code path");
      case ScanMethod::HilbertCurve: return hilbertCurve(index, blockDimensions);
      case ScanMethod::MortonCurve: return mortonCurve(index, blockDimensions);
    }
    throw "Only 'cause -Werror=return-type doesn't recognize, that this function always returns through switch";
  }
  View2D<R, true> imageView{};
  ScanMethod scanMethod{};
  Dimensions startPosition{};
  Dimensions blockDimensions{};
  NeighborModel model;
};

template<std::ranges::contiguous_range R, BlockScorer<uint8_t> Scorer, Model<uint8_t> NeighborModel>
class AdaptiveImageScanner<R, Scorer, NeighborModel>::ImageBlockIterator {
 public:
  using value_type = Block;
  using difference_type = int;
  using reference = Block &;
  ImageBlockIterator() = default;
  ImageBlockIterator(const ImageBlockIterator &other) = default;
  ImageBlockIterator &operator=(const ImageBlockIterator &other) = default;
  ImageBlockIterator(ImageBlockIterator &&other) noexcept = default;
  ImageBlockIterator &operator=(ImageBlockIterator &&other) noexcept = default;
  ImageBlockIterator(AdaptiveImageScanner &scanner, std::size_t index) : scanner(&scanner), blockIndex(index) {}

  bool operator==(const ImageBlockIterator &rhs) const { return block == rhs.block; }
  bool operator!=(const ImageBlockIterator &rhs) const { return !(rhs == *this); }
  bool operator==(const ImageBlockIteratorSentinel &other) const { return blockIndex == other.getSize(); }
  [[nodiscard]] reference operator*() const {
    if (!isInit) {
      isInit = true;
      block = scanner->getBestBlockForIndex(blockIndex);
    }
    return block;
  }
  ImageBlockIterator &operator++() {
    isInit = false;
    ++blockIndex;
    return *this;
  }
  ImageBlockIterator operator++(int) {
    auto result = *this;
    operator++();
    return result;
  }

 private:
  std::observer_ptr<AdaptiveImageScanner> scanner = nullptr;
  mutable Block block;
  std::size_t blockIndex{};
  mutable bool isInit = false;
};

template<std::ranges::contiguous_range R, BlockScorer<uint8_t> Scorer, Model<uint8_t> NeighborModel>
struct AdaptiveImageScanner<R, Scorer, NeighborModel>::ImageBlockIteratorSentinel {
 public:
  ImageBlockIteratorSentinel() = default;
  explicit ImageBlockIteratorSentinel(size_t size) : size(size) {}

  [[nodiscard]] std::size_t getSize() const { return size; }

 private:
  std::size_t size;
};

static_assert(std::forward_iterator<AdaptiveImageScanner<std::vector<int>, NeighborDifferenceScorer,
                                                         IdentityModel<uint8_t>>::ImageBlockIterator>);
static_assert(std::sentinel_for<AdaptiveImageScanner<std::vector<int>, NeighborDifferenceScorer,
                                                     IdentityModel<uint8_t>>::ImageBlockIteratorSentinel,
                                AdaptiveImageScanner<std::vector<int>, NeighborDifferenceScorer,
                                                     IdentityModel<uint8_t>>::ImageBlockIterator>);

static_assert(std::forward_iterator<AdaptiveImageScanner<std::vector<int>, NeighborDifferenceScorer,
                                                         IdentityModel<uint8_t>>::Block::Iterator>);
static_assert(
    std::sentinel_for<
        AdaptiveImageScanner<std::vector<int>, NeighborDifferenceScorer, IdentityModel<uint8_t>>::Block::Sentinel,
        AdaptiveImageScanner<std::vector<int>, NeighborDifferenceScorer, IdentityModel<uint8_t>>::Block::Iterator>);

}// namespace pf::kko

#endif//HUFF_CODEC__ADAPTIVEIMAGESCANNER_H

//
// Created by petr on 3/26/21.
//
#include "adaptive_blocks_decoding.h"
#include "adaptive_blocks_encoding.h"
#include "BinaryEncoder.h"
#include "Tree.h"
#include "utils.h"
#include <iostream>
#define ANKERL_NANOBENCH_IMPLEMENT
#include "AdaptiveImageScanner.h"
#include "View2D.h"
#include "adaptive_encoding.h"
#include "models.h"
#include "static_encoding.h"
#include <fmt/ostream.h>
#include <nanobench.h>
#include <tl/expected.hpp>

using namespace pf::kko;


int main() {
  // clang-format off
  std::vector<uint8_t> data {
      1,2,3,4,5,6,7,8,  1,1,1,1,1,1,1,1,
      1,2,3,4,5,6,7,8,  2,2,2,2,2,2,2,2,
      1,2,3,4,5,6,7,8,  3,3,3,3,3,3,3,3,
      1,2,3,4,5,6,7,8,  4,4,4,4,4,4,4,4,
      1,2,3,4,5,6,7,8,  5,5,5,5,5,5,5,5,
      1,2,3,4,5,6,7,8,  6,6,6,6,6,6,6,6,
      1,2,3,4,5,6,7,8,  7,7,7,7,7,7,7,7,
      1,2,3,4,5,6,7,8,  8,8,8,8,8,8,8,8,

      1,  2,  6,  7, 15, 16, 28, 29,        1,  2,  3,  4,  6,  7,  8,  9,
      3,  5,  8, 14, 17, 27, 30, 43,        2,  3,  4,  5,  7,  8,  9, 10,
      4,  9, 13, 18, 26, 31, 42, 44,        3,  4,  5,  7,  8,  9, 10, 11,
      10, 12, 19, 25, 32, 41, 45, 54,       4,  5,  7,  8,  9, 10, 11, 12,
      11, 20, 24, 33, 40, 46, 53, 55,       5,  7,  8,  9, 10, 11, 12, 13,
      21, 23, 34, 39, 47, 52, 56, 61,       7,  8,  9, 10, 11, 12, 13, 14,
      22, 35, 38, 48, 51, 57, 60, 62,       8,  9, 10, 11, 12, 13, 14, 15,
      36, 37, 49, 50, 58, 59, 63, 64,       9, 10, 11, 12, 13, 14, 15, 16,


  };
  // clang-format on

  /*
  auto scanner = AdaptiveImageScanner(data, 16, {8, 8}, SameNeighborsScorer{}, IdentityModel<uint8_t>{});

  for (auto &block : scanner) {
    std::size_t cnt = 0;
    std::cout << magic_enum::enum_name(block.getScanMethod()) << std::endl;
    for (auto symbol : block) {
      std::cout << static_cast<int>(symbol) << "\t";
      ++cnt;
      if (cnt % scanner.getBlockDimensions().first == 0) {
        std::cout << std::endl;
      }
    }
    std::cout << std::endl;
  }

  std::cout << "__________________________________\n";

  auto scanner2 = AdaptiveImageScanner(data, 16, {8, 8}, SameNeighborsScorer{}, NeighborDifferenceModel<uint8_t>{});

  for (auto &block : scanner2) {
    std::size_t cnt = 0;
    std::cout << magic_enum::enum_name(block.getScanMethod()) << std::endl;
    for (auto symbol : block) {
      std::cout << static_cast<int>(symbol) << "\t";
      ++cnt;
      if (cnt % scanner2.getBlockDimensions().first == 0) {
        std::cout << std::endl;
      }
    }
    std::cout << std::endl;
  }
*/
  /*
  std::mt19937 rng(1);
  std::discrete_distribution<> dist({1,2});
  std::ranges::generate_n(std::back_inserter(data), 1024 * 1024, [&] { return static_cast<uint8_t>(dist(rng)); });

  auto adaptiveBlocks = encodeImageAdaptiveBlocks<uint8_t>(data, 1024);
  auto adaptiveBlocksModel = encodeImageAdaptiveBlocks<uint8_t>(data, 1024, NeighborDifferenceModel<uint8_t>{});
  auto adaptive = encodeAdaptive<uint8_t>(data);
  auto adaptiveModel = encodeAdaptive<uint8_t>(data, NeighborDifferenceModel<uint8_t>{});
  auto stat = encodeStatic<uint8_t>(data);

  std::cout << "data: " << data.size() << std::endl;
  std::cout << "adaptiveBlocks: " << adaptiveBlocks.size() << std::endl;
  std::cout << "adaptiveBlocksModel: " << adaptiveBlocksModel.size() << std::endl;
  std::cout << "adaptive: " << adaptive.size() << std::endl;
  std::cout << "adaptiveModel: " << adaptiveModel.size() << std::endl;
  std::cout << "static: " << stat.size() << std::endl;
   */

  auto adaptiveBlocks = encodeImageAdaptiveBlocks<uint8_t>(data, 16, NeighborDifferenceModel<uint8_t>{});

  auto result = decodeImageAdaptiveBlocks<uint8_t>(adaptiveBlocks, NeighborDifferenceModel<uint8_t>{});
  for (int i = 0; i < 16; ++i) {
    for (int j = 0; j < 16; ++j) { std::cout << static_cast<int>(result.value()[i * 16 + j]) << "\t"; }
    std::cout << std::endl;
  }

  return 0;
}
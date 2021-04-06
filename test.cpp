//
// Created by petr on 3/26/21.
//
#define FMT_HEADER_ONLY
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

 //for (std::size_t i = 0; i < 64; ++i) {
 //  const auto pos = mortonCurve(i, {8, 8});
 //  fmt::print("{}x{} ", pos.first, pos.second);
 //}
 //return 0;

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

       1,  1,  1,  1,  5,  5,  8,  8,       1,  2,  3,  4,  6,  7,  8,  9,
       1,  1,  1,  1,  5,  5,  8,  8,       2,  3,  4,  5,  7,  8,  9, 10,
       2,  2,  3,  3,  6,  6,  7,  7,       3,  4,  5,  7,  8,  9, 10, 11,
       2,  2,  3,  3,  6,  6,  7,  7,       4,  5,  7,  8,  9, 10, 11, 12,
      12, 12, 11, 11, 10, 10,  9,  9,       5,  7,  8,  9, 10, 11, 12, 13,
      12, 12, 11, 11, 10, 10,  9,  9,       7,  8,  9, 10, 11, 12, 13, 14,
      13, 13, 14, 14, 15, 15, 16, 16,       8,  9, 10, 11, 12, 13, 14, 15,
      13, 13, 14, 14, 15, 15, 16, 16,       9, 10, 11, 12, 13, 14, 15, 16,


  };
  // clang-format on


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

  //auto adaptiveBlocks = encodeImageAdaptiveBlocks<uint8_t>(data, 16, NeighborDifferenceModel<uint8_t>{});
//
  //auto result = decodeImageAdaptiveBlocks<uint8_t>(adaptiveBlocks, NeighborDifferenceModel<uint8_t>{});
  //for (int i = 0; i < 16; ++i) {
  //  for (int j = 0; j < 16; ++j) { std::cout << static_cast<int>(result.value()[i * 16 + j]) << "\t"; }
  //  std::cout << std::endl;
  //}

  return 0;
}
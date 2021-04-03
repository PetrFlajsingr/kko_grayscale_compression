/**
 * @name adaptive_blocks_encoding.h
 * @brief encoding of supplied data using adaptive huffman encoding and adaptive image scanning
 * @author Petr Flajšingr, xflajs00
 * @date 3.4.2021
 */

#ifndef HUFF_CODEC__ENCODE_ADAPTIVE_BLOCKS_H
#define HUFF_CODEC__ENCODE_ADAPTIVE_BLOCKS_H

#include "AdaptiveImageScanner.h"
#include "BinaryEncoder.h"
#include "adaptive_common.h"
#include "models.h"
#include "utils.h"
#include <concepts>
#include <ranges>
#include <vector>

namespace pf::kko {

/**
 * Model is reset for each block
 * header: 16 bit width, 16 bit height, 8 bit block width, 8 bit block height
 * block header: 4 bit scan method - 0b1111 is end
 * adaptive hamming code(AHC) -> block header(BH) -> AHC -> BH... -> BH 0b1111
 * blocks are padded with zeros
 * @param data data to be encoded
 * @param imageWidth width of image
 * @param model transformation of neighboring data
 * @return data encoded using adaptive huffman code
 */
template<std::integral T>
std::vector<uint8_t> encodeImageAdaptiveBlocks(std::ranges::forward_range auto &&data, std::size_t imageWidth,
                                               Model<T> auto &&model) {
  auto view = makeView2D<true>(data, imageWidth);

  auto scanner = AdaptiveImageScanner(view, {8, 8}, SameNeighborsScorer{}, std::move(model));

  auto symbolNodes = detail::NodeCacheArray<T>{nullptr};
  auto tree = detail::TreeType<T>{};

  tree.setRoot(makeUniqueNode(makeNYTAdaptive<T>()));
  auto nytNode = std::make_observer(&tree.getRoot());
  spdlog::trace("Tree initialised");

  auto binEncoder = BinaryEncoder<uint8_t>{};
  const auto imageHeight = view.size();
  // save image info - image size, block size
  binEncoder.pushBack(static_cast<uint16_t>(imageWidth),
                      static_cast<uint16_t>(imageHeight));// TODO: check evaluation order on merlin
  binEncoder.pushBack(
      static_cast<uint8_t>(scanner.getBlockDimensions().first),
      static_cast<uint8_t>(scanner.getBlockDimensions().second));// TODO: check evaluation order on merlin
  spdlog::trace("Added header");

  for (auto &block : scanner) {
    // save block info (scan method type)
    binEncoder.pushBack(typeToBits(block.getScanMethod(), 4));

    for (auto symbol : block) {
      auto code = std::vector<bool>{};
      if (symbolNodes[symbol] != nullptr) {
        code = getPathToSymbol<T>(tree.getRoot(), *symbolNodes[symbol]);
      } else {
        code = getPathToSymbol<T>(tree.getRoot(), *nytNode);
        std::ranges::copy(typeToBits(symbol, 9), std::back_inserter(code));
      }
      binEncoder.pushBack(code);
      nytNode = updateTree(tree, symbol, nytNode, symbolNodes);
    }
  }
  binEncoder.pushBack(typeToBits(uint8_t{0b1111}, 4));
  spdlog::info("Done, output data size: %[b]", binEncoder.size());

  return binEncoder.releaseData();
}

}// namespace pf::kko

#endif//HUFF_CODEC__ENCODE_ADAPTIVE_BLOCKS_H

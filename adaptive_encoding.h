/**
 * @name adaptive_encoding.h
 * @brief functions for adaptive huffman encoding
 * @author Petr Flajšingr, xflajs00
 * @date 30.03.2021
 */
#ifndef HUFF_CODEC__ADAPTIVE_ENCODING_H
#define HUFF_CODEC__ADAPTIVE_ENCODING_H

#include "BinaryEncoder.h"
#include "EncodingTreeData.h"
#include "Tree.h"
#include "adaptive_common.h"
#include "models.h"
#include <algorithm>

namespace pf::kko {

namespace detail {
template<std::integral T>
bool getPathToSymbolImpl(detail::NodeType<T> &root, detail::NodeType<T> &targetNode, std::vector<bool> &result) {
  if (&root == &targetNode) { return true; }
  if (root.hasLeft()) {
    if (getPathToSymbolImpl(root.getLeft(), targetNode, result)) {
      result.emplace_back(false);
      return true;
    }
  }
  if (result.empty() && root.hasRight()) {
    if (getPathToSymbolImpl(root.getRight(), targetNode, result)) {
      result.emplace_back(true);
      return true;
    }
  }
  return false;
}
}// namespace detail

template<std::integral T>
std::vector<bool> getPathToSymbol(detail::NodeType<T> &root, detail::NodeType<T> &targetNode) {
  if (&root == &targetNode) { return {}; }
  auto result = std::vector<bool>{};
  if (root.hasLeft()) {
    if (detail::getPathToSymbolImpl(root.getLeft(), targetNode, result)) { result.emplace_back(false); }
  }
  if (result.empty() && root.hasRight()) {
    if (detail::getPathToSymbolImpl(root.getRight(), targetNode, result)) { result.emplace_back(true); }
  }
  std::reverse(result.begin(), result.end());
  return result;
}

/**
 * Vitter algorithm implementation of adaptive huffman encoding.
 * @param data data to be encoded
 * @param model
 * @return encoded data using adaptive huffman encoding
 */
template<std::integral T>
std::vector<uint8_t> encodeAdaptive(std::ranges::forward_range auto &&data, Model<T> auto &&model) {
  std::ranges::transform(data, std::ranges::begin(data), makeApplyLambda<T>(model));
  // speedup for leaf node lookup - no need to traverse the tree
  auto symbolNodes = detail::NodeCacheArray<T>{nullptr};

  auto tree = detail::TreeType<T>{};

  tree.setRoot(makeUniqueNode(makeNYTAdaptive<T>()));
  auto nytNode = std::make_observer(&tree.getRoot());
  spdlog::trace("Tree initialised");

  auto binEncoder = BinaryEncoder<uint8_t>{};

  for (auto symbol : data) {
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
  // adding PSEUDO_EOF
  const auto eofCode = getPathToSymbol<T>(tree.getRoot(), *nytNode);
  binEncoder.pushBack(eofCode);
  spdlog::info("Done, output data size: {}[b]", binEncoder.size());

  return binEncoder.releaseData();
}
}// namespace pf::kko
#endif//HUFF_CODEC__ADAPTIVE_ENCODING_H

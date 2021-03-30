//
// Created by petr on 3/28/21.
//

#ifndef HUFF_CODEC__ADAPTIVE_ENCODING_H
#define HUFF_CODEC__ADAPTIVE_ENCODING_H

#include "BinaryEncoder.h"
#include "EncodingTreeData.h"
#include "Tree.h"
#include "models.h"
#include <algorithm>

namespace pf::kko {

namespace detail {
template<std::integral T>
using NodeType = Node<AdaptiveEncodingTreeData<T>>;
template<std::integral T>
using TreeType = Tree<AdaptiveEncodingTreeData<T>>;
template<std::integral T>
using NodeCacheArray = std::array<std::observer_ptr<detail::NodeType<T>>, ValueCount<T>>;
}// namespace detail

/**
 * Traverse tree and save path taken, where left is false a right is true
 * @param root trees root
 * @param targetNode searched for node
 * @param pathPrefix path taken thus far
 * @return path from provided root node to target node
 */
template<std::integral T>
std::vector<bool> getPathToSymbol(detail::NodeType<T> &root, detail::NodeType<T> &targetNode,
                                  std::vector<bool> pathPrefix = {}) {
  if (&root == &targetNode) { return pathPrefix; }
  auto result = std::vector<bool>{};
  if (root.hasLeft()) {
    auto leftPathPrefix = pathPrefix;
    leftPathPrefix.template emplace_back(false);
    result = getPathToSymbol(root.getLeft(), targetNode, leftPathPrefix);
  }
  if (result.empty() && root.hasRight()) {
    auto rightPathPrefix = pathPrefix;
    rightPathPrefix.template emplace_back(true);
    result = getPathToSymbol(root.getRight(), targetNode, rightPathPrefix);
  }
  return result;
}

template<std::integral T>
std::observer_ptr<detail::NodeType<T>> findNodeForSwap(std::observer_ptr<detail::NodeType<T>> root,
                                                       std::observer_ptr<detail::NodeType<T>> node,
                                                       std::observer_ptr<detail::NodeType<T>> inspectedNode) {
  if (inspectedNode == root) { return inspectedNode; }

  if ((*inspectedNode)->weight == (*node)->weight && node != inspectedNode->getParent()
      && (*inspectedNode)->order < (*node)->order) {
    inspectedNode = node;
  }

  if (node->hasLeft()) { inspectedNode = findNodeForSwap(root, std::make_observer(&node->getLeft()), inspectedNode); }
  if (node->hasRight()) { inspectedNode = findNodeForSwap(root, std::make_observer(&node->getRight()), inspectedNode); }
  return inspectedNode;
}

template<std::integral T>
void slideAndIncrement(detail::TreeType<T> &tree, detail::NodeType<T> &node) {
  auto root = std::make_observer(&tree.getRoot());
  auto nodeToSwapWith = findNodeForSwap(root, root, std::make_observer(&node));
  if (nodeToSwapWith.get() != &node) {
    swapNodes(nodeToSwapWith, std::make_observer(&node));
    std::swap((*nodeToSwapWith)->order, node->order);
  }

  ++node->weight;
  if (!node.isRoot()) { slideAndIncrement(tree, *node.getParent()); }
}

template<std::integral T>
std::observer_ptr<detail::NodeType<T>> updateTree(detail::TreeType<T> &tree, T symbol,
                                                  std::observer_ptr<detail::NodeType<T>> nytNode,
                                                  detail::NodeCacheArray<T> &nodeCacheArray) {
  auto symbolNode = nodeCacheArray[symbol];
  if (symbolNode == nullptr) {
    (*nytNode)->isNYT = false;
    const auto currentOrder = (*nytNode)->order;
    auto &left = nytNode->makeLeft(makeNYTAdaptive<T>(currentOrder - 2));
    auto &right = nytNode->makeRight(AdaptiveEncodingTreeData<T>{symbol, 0, false, currentOrder - 1});
    symbolNode = std::make_observer(&right);
    nodeCacheArray[symbol] = symbolNode;
    nytNode = std::make_observer(&left);
  }

  slideAndIncrement(tree, *symbolNode);

  return nytNode;
}

template<std::integral T, typename Model = IdentityModel<T>>
std::vector<uint8_t> encodeAdaptive(std::ranges::forward_range auto &&data, Model &&model = Model{}) {
  std::ranges::transform(data, std::ranges::begin(data), makeApplyLambda<T>(model));
  // speedup for leaf node lookup - no need to traverse the tree
  auto symbolNodes = detail::NodeCacheArray<T>{nullptr};

  auto tree = detail::TreeType<T>{};

  tree.setRoot(makeUniqueNode(makeNYTAdaptive<T>()));
  auto nytNode = std::make_observer(&tree.getRoot());

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
  return binEncoder.releaseData();
}
}// namespace pf::kko
#endif//HUFF_CODEC__ADAPTIVE_ENCODING_H

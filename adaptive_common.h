/**
 * @name adaptive_common.h
 * @brief common functions and types for both adaptive encoding and decoding
 * @author Petr Flajšingr, xflajs00
 * @date 31.03.2021
 */

#ifndef HUFF_CODEC__ADAPTIVE_COMMON_H
#define HUFF_CODEC__ADAPTIVE_COMMON_H

#include "EncodingTreeData.h"
namespace pf::kko {

namespace detail {
template<std::integral T>
using NodeType = Node<AdaptiveEncodingTreeData<T>>;
template<std::integral T>
using TreeType = Tree<AdaptiveEncodingTreeData<T>>;
template<std::integral T>
using NodeCacheArray = std::array<std::observer_ptr<detail::NodeType<T>>, ValueCount<T>>;
}// namespace detail

template <std::integral T>
constexpr auto PSEUDO_EOF = ~T{0};

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

}
#endif//HUFF_CODEC__ADAPTIVE_COMMON_H

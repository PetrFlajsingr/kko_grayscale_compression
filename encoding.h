//
// Created by petr on 3/23/21.
//

#ifndef PF_HUFF_CODEC__COMPRESSION_H
#define PF_HUFF_CODEC__COMPRESSION_H

#include "Iterable.h"
#include "Tree.h"
#include "utils.h"
#include <algorithm>
#include <concepts>
#include <queue>
#include <vector>

template<typename T, typename ValueType>
concept Model = std::invocable<T, ValueType> &&std::same_as<std::invoke_result_t<T, ValueType>, ValueType>;

template<typename T>
struct IdentityModel {
  T operator()(T &value) { return value; }
};

template<typename T>
struct NeighborDifferenceModel {
  T operator()(T &value) {
    const auto result = value - lastVal;
    lastVal = value;
    return result;
  }
  T lastVal{};
};

template<std::integral T>
std::array<std::size_t, constexpr_pow(2, sizeof(T) * 8)> createHistogram(pf::Iterable auto &data) {
  auto result = std::array<std::size_t, constexpr_pow(2, sizeof(T) * 8)>{};
  std::ranges::for_each(data, [&result](const auto value) { ++result[value]; });
  return result;
}

template<std::integral T>
struct TreeData {
  T value;
  std::size_t weight;
  bool isValid;

  bool operator<(const TreeData &rhs) const { return weight < rhs.weight; }
  bool operator>(const TreeData &rhs) const { return rhs < *this; }
  bool operator<=(const TreeData &rhs) const { return !(rhs < *this); }
  bool operator>=(const TreeData &rhs) const { return !(*this < rhs); }
};

template<std::integral T>
Tree<TreeData<T>> buildTree(const pf::Iterable auto &histogram) {
  using NodeType = Node<TreeData<T>>;
  auto treeHeap = std::priority_queue<PriorityQueueMoveHack<std::unique_ptr<NodeType>>>();
  auto counter = T{};
  std::ranges::for_each(histogram, [&treeHeap, &counter](const auto &occurrenceCnt) {
    if (occurrenceCnt > 0) { treeHeap.emplace(std::make_unique<NodeType>(TreeData<T>{counter, occurrenceCnt, true})); }
    ++counter;
  });
  if (treeHeap.size() == 1) { treeHeap.emplace(std::make_unique<NodeType>(TreeData<T>{0, 1, true})); }
  while (treeHeap.size() != 1) {
    auto left = std::move(treeHeap.top().value);
    treeHeap.pop();
    auto right = std::move(treeHeap.top().value);
    treeHeap.pop();
    auto newNode = std::make_unique<NodeType>(TreeData<T>{0, left->getValue().weight + right->getValue().weight, true});
    newNode->setLeft(std::move(left));
    newNode->setRight(std::move(right));
    treeHeap.emplace(std::move(newNode));
  }
  auto result = Tree<TreeData<T>>();
  result.setRoot(std::move(treeHeap.top().value));
  return result;
}

template<std::integral T, typename Model = IdentityModel<T>>
std::vector<uint8_t> encodeStatic(pf::Iterable auto &&data, Model &&model = Model{}) {
  std::ranges::transform(data, std::begin(data), model);
  const auto histogram = createHistogram<uint8_t>(data);
  [[maybe_unused]] auto tree = buildTree<T>(histogram);
  [[maybe_unused]] auto cnt = countNodes(tree.getRoot());
  return {};
}

#endif//PF_HUFF_CODEC__COMPRESSION_H

//
// Created by petr on 3/23/21.
//

#ifndef PF_HUFF_CODEC__COMPRESSION_H
#define PF_HUFF_CODEC__COMPRESSION_H

#include "BinaryEncoder.h"
#include "Iterable.h"
#include "Tree.h"
#include "utils.h"
#include <algorithm>
#include <concepts>
#include <queue>
#include <ranges>
#include <spdlog/spdlog.h>
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
std::array<std::size_t, ValueCount<T>> createHistogram(pf::Iterable auto &data) {
  auto result = std::array<std::size_t, ValueCount<T>>{};
  std::ranges::for_each(data, [&result](const auto value) { ++result[value]; });
  return result;
}

template<std::integral T>
struct TreeData {
  T value;
  std::size_t weight;
  bool isValid;

  bool operator<(const TreeData &rhs) const { return weight < rhs.weight; }
  bool operator>(const TreeData &rhs) const { return rhs.weight < weight; }
  bool operator<=(const TreeData &rhs) const { return !(rhs < *this); }
  bool operator>=(const TreeData &rhs) const { return !(*this < rhs); }

  bool operator==(const TreeData &rhs) const { return weight == rhs.weight; }
  bool operator!=(const TreeData &rhs) const { return !(rhs == *this); }
};

template<std::integral T>
Tree<TreeData<T>> buildTree(const pf::Iterable auto &histogram) {
  using NodeType = Node<TreeData<T>>;
  const auto compare = [](const auto &lhs, const auto &rhs) { return *lhs > *rhs; };

  auto treeHeap = std::vector<std::unique_ptr<NodeType>>{};

  auto counter = T{};

  std::ranges::for_each(histogram, [&treeHeap, &counter, compare](const auto &occurrenceCnt) {
    if (occurrenceCnt > 0) {
      pushAsHeap(treeHeap, std::make_unique<NodeType>(TreeData<T>{counter, occurrenceCnt, true}), compare);
    }
    ++counter;
  });
  if (treeHeap.size() == 1) { pushAsHeap(treeHeap, std::make_unique<NodeType>(TreeData<T>{0, 1, false}), compare); }
  while (treeHeap.size() > 1) {
    auto left = popAsHeap(treeHeap, compare);
    auto right = popAsHeap(treeHeap, compare);
    auto newNode = std::make_unique<NodeType>(TreeData<T>{0, left->getValue().weight + right->getValue().weight, true});
    newNode->setLeft(std::move(left));
    newNode->setRight(std::move(right));
    treeHeap.emplace_back(std::move(newNode));
    std::push_heap(treeHeap.begin(), treeHeap.end(), compare);
  }
  auto result = Tree<TreeData<T>>();
  result.setRoot(std::move(treeHeap.front()));
  return result;
}

template<std::integral T>
void buildCodes(Node<TreeData<T>> &node, std::vector<std::pair<T, std::vector<bool>>> &codesForSymbols,
                const std::vector<bool> &branchCode) {
  if (node.isLeaf()) {
    codesForSymbols.template emplace_back(node->value, branchCode);
    return;
  }
  auto leftCode = branchCode;
  auto rightCode = branchCode;
  leftCode.template emplace_back(false);
  rightCode.template emplace_back(true);
  if (node.hasLeft()) { buildCodes(node.getLeft(), codesForSymbols, leftCode); }
  if (node.hasRight()) { buildCodes(node.getRight(), codesForSymbols, rightCode); }
}

template<std::integral T>
std::vector<std::pair<T, std::vector<bool>>> buildCodes(Node<TreeData<T>> &node) {
  auto result = std::vector<std::pair<T, std::vector<bool>>>{};
  buildCodes(node, result, {});
  return result;
}

template<std::integral T>
void transformCodes(std::vector<std::pair<T, std::vector<bool>>> &codes) {
  std::ranges::sort(codes, [](const auto &lhs, const auto &rhs) { return lhs.second.size() < rhs.second.size(); });

  auto &[unused1_, smallestBits] = codes.front();
  const auto leftBit = smallestBits.size();
  auto leftCode = std::size_t{};

  smallestBits.clear();
  std::ranges::generate_n(std::back_inserter(smallestBits), leftBit, [] { return false; });

  if (codes.size() <= 1) { return; }
  auto &[unused2_, secondSmallestBits] = codes[1];
  const auto rightBit = secondSmallestBits.size();
  auto rightCode = (leftCode + 1) << (rightBit - leftBit);
  auto prevLeftBit = rightBit;
  auto prevLeftCode = rightCode;
  secondSmallestBits.clear();

  for (; rightCode != 0; rightCode >>= 1) { secondSmallestBits.template emplace_back(rightCode & 1); }

  std::ranges::generate_n(std::back_inserter(secondSmallestBits), rightBit - secondSmallestBits.size(),
                          [] { return false; });
  std::reverse(secondSmallestBits.begin(), secondSmallestBits.end());

  for (std::size_t cnt = 2, idx = codes.size(); cnt < idx; ++cnt) {
    auto &symbol = codes[cnt];
    auto leftSecond = symbol.second.size();
    auto rightSecond = (prevLeftCode + 1) * (1 << (leftSecond - prevLeftBit));
    prevLeftBit = leftSecond;
    prevLeftCode = rightSecond;
    symbol.second.clear();
    for (; rightSecond != 0; rightSecond >>= 1) { symbol.second.template emplace_back(rightSecond & 1); }
    std::ranges::generate_n(std::back_inserter(symbol.second), leftSecond - symbol.second.size(), [] { return false; });
    std::reverse(symbol.second.begin(), symbol.second.end());
  }
}

/**
 * @details Model je concept kvuli optimalizacim - napriklad pro IdentityModel dojde k optimalizaci cele transformace
 * pryc a tim se usetri cas v runtime. Dynamickeho polymorfismu se da snadno dosahnout bez modifikace teto funkce.
 * @tparam Model
 * @param data
 * @param model
 * @return
 */
template<std::integral T, typename Model = IdentityModel<T>>
std::vector<uint8_t> encodeStatic(std::ranges::forward_range auto &&data, Model &&model = Model{}) {
  spdlog::info("Starting static encoding");
  std::ranges::transform(data, std::ranges::begin(data), model);
  spdlog::trace("Applied model");
  const auto histogram = createHistogram<uint8_t>(data);
  spdlog::trace("Created histogram");
  auto tree = buildTree<T>(histogram);
  spdlog::info("Built tree");
  auto symbolCodes = buildCodes(tree.getRoot());
  transformCodes(symbolCodes);
  spdlog::info("Created symbol codes");

  const auto [minCodeLength, maxCodeLength] =
      minmaxValue(symbolCodes | std::views::transform([](const auto &el) { return el.second.size(); })).value();

  auto byteHeader = std::vector<std::vector<uint8_t>>{};
  byteHeader.resize(maxCodeLength + 1);
  auto byteTable = std::array<std::vector<bool>, ValueCount<T>>{};

  std::ranges::for_each(symbolCodes, [&byteTable, &byteHeader](const auto &symbolCode) {
    const auto &[symbol, code] = symbolCode;
    byteTable[symbol] = code;
    byteHeader[code.size()].emplace_back(symbol);
  });
  spdlog::info("Created header");
  spdlog::trace("Start binary encoding");

  auto binEncoder = BinaryEncoder<uint8_t>{};

  const auto padding = binEncoder.unusedBitsInCell();
  const auto countThing = static_cast<uint8_t>(maxCodeLength + 1);
  const auto paddingThing = static_cast<uint8_t>((minCodeLength - 1) | (static_cast<uint8_t>(padding << 5)));

  binEncoder.pushBack(countThing);
  binEncoder.pushBack(paddingThing);

  std::ranges::for_each(byteHeader.begin() + minCodeLength, byteHeader.end(), [&binEncoder](const auto &v) {
    const auto size = static_cast<uint8_t>(v.size());
    binEncoder.pushBack(size);
  });
  std::ranges::for_each(byteHeader, [&binEncoder](auto val) { binEncoder.pushBack(val); });
  std::ranges::for_each(data, [&binEncoder, &byteTable](const auto &in) { binEncoder.pushBack(byteTable[in]); });

  binEncoder.shrinkToFit();
  spdlog::info("Data encoded, total length: {}[b]", binEncoder.size());
  return binEncoder.releaseData();
}

#endif//PF_HUFF_CODEC__COMPRESSION_H

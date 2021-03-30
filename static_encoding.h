//
// Created by petr on 3/23/21.
//

#ifndef PF_HUFF_CODEC__COMPRESSION_H
#define PF_HUFF_CODEC__COMPRESSION_H

#include "BinaryEncoder.h"
#include "EncodingTreeData.h"
#include "Tree.h"
#include "constants.h"
#include "models.h"
#include "utils.h"
#include <algorithm>
#include <concepts>
#include <queue>
#include <ranges>
#include <spdlog/spdlog.h>
#include <vector>

// TODO: EOF misto poctu b
namespace pf::kko {

template<std::integral T>
std::array<std::size_t, ValueCount<T>> createHistogram(const std::ranges::forward_range auto &data) {
  auto result = std::array<std::size_t, ValueCount<T>>{};
  std::ranges::for_each(data, [&result](const auto value) { ++result[value]; });
  return result;
}

/**
 * Create a huffman tree for encoding purposes.
 * @details stl heap functions are used for code simplification
 * @param histogram histogram of occurrences in input data
 * @return binary tree, where leaves represent symbols
 */
template<std::integral T>
Tree<StaticEncodingTreeData<T>> buildTree(const std::ranges::forward_range auto &histogram) {
  using NodeType = Node<StaticEncodingTreeData<T>>;
  // comparison function dereferencing ptrs
  const auto compare = [](const auto &lhs, const auto &rhs) { return *lhs > *rhs; };

  auto treeHeap = std::vector<std::unique_ptr<NodeType>>{};

  // counter representing an encoded symbol, could be replaced with ranges library (histogram | views::enumerate)
  std::integral auto counter = T{};
  std::ranges::for_each(histogram, [&treeHeap, &counter, compare](const auto &occurrenceCnt) {
    if (occurrenceCnt > 0) {
      pushAsHeap(treeHeap, makeUniqueNode(StaticEncodingTreeData<T>{counter, occurrenceCnt, true}), compare);
    }
    ++counter;
  });
  // if the tree only has one element, add special node
  if (treeHeap.size() == 1) { pushAsHeap(treeHeap, makeUniqueNode(makeNYTStatic<T>()), compare); }
  // heap transformation to a binary tree
  while (treeHeap.size() > 1) {
    auto left = popAsHeap(treeHeap, compare);
    auto right = popAsHeap(treeHeap, compare);
    auto newNode = makeUniqueNode(StaticEncodingTreeData<T>{0, left->getValue().weight + right->getValue().weight, true});
    newNode->setLeft(std::move(left));
    newNode->setRight(std::move(right));
    treeHeap.emplace_back(std::move(newNode));
    std::push_heap(treeHeap.begin(), treeHeap.end(), compare);
  }
  auto result = Tree<StaticEncodingTreeData<T>>();
  result.setRoot(std::move(treeHeap.front()));
  return result;
}

/**
 * Implementation of huffman code creation.
 * @param node currently inspected node
 * @param codesForSymbols storage for results
 * @param branchCode code from the previous levels of the tree
 */
template<std::integral T>
void buildCodes(Node<StaticEncodingTreeData<T>> &node, std::vector<std::pair<T, std::vector<bool>>> &codesForSymbols,
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

/**
 * Create huffman code for each symbol.
 * @param node tree root node
 * @return vector of pairs, where pair.first is a symbol and pair.second is binary code
 */
template<std::integral T>
std::vector<std::pair<T, std::vector<bool>>> buildCodes(Node<StaticEncodingTreeData<T>> &node) {
  auto result = std::vector<std::pair<T, std::vector<bool>>>{};
  buildCodes(node, result, {});
  return result;
}

/**
 * Make code canonical in order to save some data space
 * @param codes codes from buildCodes function
 */
template<std::integral T>
void transformCodes(std::vector<std::pair<T, std::vector<bool>>> &codes) {
  // sort the symbols by their code length
  std::ranges::sort(codes, [](const auto &lhs, const auto &rhs) { return lhs.second.size() < rhs.second.size(); });

  auto &[unused1_, shortestCode] = codes.front();
  const auto shortestCodeLen = shortestCode.size();

  shortestCode.clear();
  shortestCode.resize(shortestCodeLen, false);

  if (codes.size() <= 1) { return; }

  auto &[unused2_, secondShortestCode] = codes[1];
  const auto secondShortestCodeOrigLen = secondShortestCode.size();
  auto previousCodeLen = secondShortestCodeOrigLen;
  auto previousCode = 1 << (secondShortestCodeOrigLen - shortestCodeLen);

  const auto fillWithZerosAndOnePadAndReverse = [](auto &vec, auto mask, auto padLen) {
    for (; mask != 0; mask >>= 1) { vec.template emplace_back(mask & 1); }
    std::ranges::generate_n(std::back_inserter(vec), padLen - vec.size(), [] { return false; });
    std::reverse(vec.begin(), vec.end());
  };

  secondShortestCode.clear();
  fillWithZerosAndOnePadAndReverse(secondShortestCode, previousCode, secondShortestCodeOrigLen);

  for (std::size_t cnt = 2; cnt < codes.size(); ++cnt) {
    auto &symbol = codes[cnt];
    const auto codeLen = symbol.second.size();
    const auto code = (previousCode + 1) * (1 << (codeLen - previousCodeLen));
    previousCodeLen = codeLen;
    previousCode = code;
    symbol.second.clear();
    fillWithZerosAndOnePadAndReverse(symbol.second, code, codeLen);
  }
}

/**
 * Encode data using prepared symbol codes.
 * @param data input data
 * @param symbolCodes huffman codes for symbol
 * @return encoded data
 */
template<std::integral T>
std::vector<uint8_t> encodeStatic_impl(std::ranges::forward_range auto &&data,
                                       const std::vector<std::pair<T, std::vector<bool>>> &symbolCodes) {
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

  auto binEncoderData = BinaryEncoder<uint8_t>{};

  std::ranges::for_each(byteHeader.begin() + minCodeLength, byteHeader.end(), [&binEncoderData](const auto &v) {
    const auto size = static_cast<uint8_t>(v.size());
    binEncoderData.pushBack(size);
  });
  std::ranges::for_each(byteHeader, [&binEncoderData](auto val) { binEncoderData.pushBack(val); });
  std::ranges::for_each(data,
                        [&binEncoderData, &byteTable](const auto &in) { binEncoderData.pushBack(byteTable[in]); });

  const auto padding = binEncoderData.unusedBitsInCell();
  const auto codeLengthInfo = static_cast<uint8_t>(maxCodeLength + 1);
  const auto paddingInfo = static_cast<uint8_t>((minCodeLength - 1) | (static_cast<uint8_t>(padding << PADDING_SHIFT)));

  auto result = std::vector<uint8_t>{};
  result.template emplace_back(codeLengthInfo);
  result.template emplace_back(paddingInfo);
  result.resize(result.size() + binEncoderData.data().size());
  std::ranges::copy(binEncoderData.releaseData(), result.begin() + 2);

  spdlog::info("Data encoded, total length: {}[b]", result.size());
  return result;
}

/**
 * @details Model is a concept for the sake of optimisations
 * @param data data to be encoded
 * @param model
 * @return encoded data
 */
template<std::integral T, typename Model = IdentityModel<T>>
std::vector<uint8_t> encodeStatic(std::ranges::forward_range auto &&data, Model &&model = Model{}) {
  spdlog::info("Starting static encoding");
  std::ranges::transform(data, std::ranges::begin(data), makeApplyLambda<T>(model));
  spdlog::trace("Applied model");
  const auto histogram = createHistogram<uint8_t>(data);
  spdlog::trace("Created histogram");
  auto tree = buildTree<T>(histogram);
  spdlog::info("Built tree");
  auto symbolCodes = buildCodes(tree.getRoot());
  spdlog::trace("Built codes");
  transformCodes(symbolCodes);
  spdlog::info("Created symbol codes");

  return encodeStatic_impl(data, symbolCodes);
}
}// namespace pf::kko

#endif//PF_HUFF_CODEC__COMPRESSION_H

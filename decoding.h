//
// Created by petr on 3/23/21.
//

#ifndef PF_HUFF_CODEC__DECOMPRESSION_H
#define PF_HUFF_CODEC__DECOMPRESSION_H

#include "constants.h"
#include "models.h"
#include "utils.h"
#include <fmt/core.h>
#include <numeric>
#include <ranges>
#include <spdlog/spdlog.h>
#include <tl/expected.hpp>

namespace pf::kko {

/**
 * Decode data encoded via @see encodeStatic<T> function
 * @tparam Model model used during data encoding
 * @param data input data
 * @return unexpected when error occurs, otherwise decoded data
 */
template<std::integral T, typename Model = IdentityModel<T>>
tl::expected<std::vector<T>, std::string> decodeStatic(std::ranges::forward_range auto &&data,
                                                       Model &&model = IdentityModel<T>()) {
  auto iter = std::ranges::begin(data);
  const auto dataSize = std::ranges::size(data);

  const auto maxCodeLengthInfo = static_cast<std::size_t>(*iter++);

  if (dataSize < maxCodeLengthInfo + 1) { return tl::make_unexpected("File size doesn't match data"); }
  const auto paddingInfo = *iter++;
  const auto padding = (paddingInfo & PADDING_MASK) >> PADDING_SHIFT;
  const auto minCodeLengthInfo = paddingInfo & (~PADDING_MASK);
  const auto lenDiff = maxCodeLengthInfo - minCodeLengthInfo;

  auto byteLengths = std::vector<std::size_t>{};
  byteLengths.resize(minCodeLengthInfo, 0);

  auto symbolsLength = std::size_t{};
  for (auto i = lenDiff + 1; iter != std::ranges::begin(data) + i;) {
    auto indexLength = static_cast<std::size_t>(*iter++);
    if (indexLength == 0 && minCodeLengthInfo == 7) { indexLength = 256; }
    byteLengths.template emplace_back(indexLength);
    symbolsLength += indexLength;
  }

  if (dataSize < lenDiff + symbolsLength + 1) { return tl::make_unexpected("Not enough data"); }

  const auto byteCount = (lenDiff + 1 + symbolsLength);
  const auto currentIterPos = std::ranges::distance(std::ranges::begin(data), iter);
  auto symbols = std::vector<std::size_t>(iter, iter + (byteCount - currentIterPos));
  iter += byteCount - currentIterPos;

  auto counterOnes = std::size_t{1};
  auto counterZeros = std::size_t{};

  auto zeros = std::vector<std::size_t>{counterZeros};
  auto ones = std::vector<std::size_t>{counterOnes};
  std::ranges::for_each(byteLengths, [&](const auto byteLength) {
    counterOnes += byteLength;
    counterZeros += byteLength;
    counterZeros *= 2;
    zeros.template emplace_back(counterZeros);
    ones.template emplace_back(counterOnes);
  });

  auto result = std::vector<T>{};
  auto readBytesCounter = static_cast<std::size_t>(std::ranges::distance(data.begin(), iter));
  counterOnes = 0;
  counterZeros = 0;
  try {
    std::for_each(iter, std::ranges::end(data), [&](const auto byte) {
      const auto usedBitsInByte = readBytesCounter == dataSize - 1 ? 8 - padding : 8;
      auto bitCnt = 0;
      forEachBit_n(byte, usedBitsInByte, [&](bool bit) {
        counterOnes += 1;
        counterZeros *= 2;
        counterZeros += (bit ? 1 : 0);
        if (counterZeros * 2 < zeros[counterOnes]) {
          const auto index = ones[counterOnes - 1] + counterZeros - zeros[counterOnes - 1] - 1;
          if (index >= symbolsLength) { throw std::runtime_error("Invalid code in input data"); }
          result.emplace_back(symbols[index]);
          counterOnes = 0;
          counterZeros = 0;
        }
        ++bitCnt;
      });
      ++readBytesCounter;
    });
  } catch (const std::runtime_error &e) { return tl::make_unexpected(e.what()); }
  std::ranges::transform(result, std::ranges::begin(result), makeRevertLambda<T>(model));
  return result;
}
}// namespace pf::kko

#endif//PF_HUFF_CODEC__DECOMPRESSION_H

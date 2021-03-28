//
// Created by petr on 3/23/21.
//

#ifndef PF_HUFF_CODEC__DECOMPRESSION_H
#define PF_HUFF_CODEC__DECOMPRESSION_H

#include "models.h"
#include "utils.h"
#include <fmt/core.h>
#include <numeric>
#include <ranges>
#include <spdlog/spdlog.h>
#include <tl/expected.hpp>

namespace pf::kko {

template<std::integral T, typename Model = IdentityModel<T>>
tl::expected<std::vector<T>, std::string> decodeStatic(std::ranges::forward_range auto &&data,
                                                       Model &&model = IdentityModel<T>()) {
  constexpr auto PADDING_MASK = 0b11100000;
  constexpr auto PADDING_SHIFT = 5;
  auto iter = std::ranges::begin(data);
  const auto dataSize = std::ranges::size(data);

  auto actualBit = static_cast<std::size_t>(*iter++);

  if (dataSize < actualBit + 1) { return tl::make_unexpected("File size doesn't match data"); }
  auto initLengthCounter = *iter++;
  [[maybe_unused]] const auto padding = (initLengthCounter & PADDING_MASK) >> PADDING_SHIFT;
  initLengthCounter &= ~PADDING_MASK;
  actualBit -= initLengthCounter;

  auto byteLengths = std::vector<std::size_t>{};
  byteLengths.resize(initLengthCounter, 0);

  auto symbolsLength = std::size_t{};
  for (auto i = actualBit + 1; iter != std::ranges::begin(data) + i;) {
    auto indexLength = static_cast<std::size_t>(*iter++);
    if (indexLength == 0 && initLengthCounter == 7) { indexLength = 256; }
    byteLengths.template emplace_back(indexLength);
    symbolsLength += indexLength;
  }

  if (dataSize < actualBit + symbolsLength + 1) { return tl::make_unexpected("Not enough data"); }

  const auto byteCount = (actualBit + 1 + symbolsLength);
  const auto currentIterPos = std::ranges::distance(std::ranges::begin(data), iter);
  auto otherSymbols = std::vector<std::size_t>(iter, iter + (byteCount - currentIterPos));
  iter += byteCount - currentIterPos;

  auto counterOnes = std::size_t{1};
  auto counterZeros = std::size_t{};

  auto beginCode = std::vector<std::size_t>{counterZeros};
  auto beginSymbol = std::vector<std::size_t>{counterOnes};
  std::ranges::for_each(byteLengths, [&](const auto byteLength) {
    counterOnes += byteLength;
    counterZeros += byteLength;
    counterZeros *= 2;
    beginCode.template emplace_back(counterZeros);
    beginSymbol.template emplace_back(counterOnes);
  });

  auto result = std::vector<T>{};
  auto readBytesCounter = static_cast<std::size_t>(std::ranges::distance(data.begin(), iter));
  auto onesCounter = std::size_t{};
  auto zerosCounter = std::size_t{};
  using namespace std::string_literals;
  std::for_each(iter, std::ranges::end(data), [&](const auto byte) {
    const auto usedBitsInByte = readBytesCounter == dataSize - 1 ? 8 - padding : 8;
    auto bitCnt = 0;
    forEachBit_n(byte, usedBitsInByte, [&](bool bit) {
      onesCounter += 1;
      zerosCounter *= 2;
      zerosCounter += (bit ? 1 : 0);
      if ((zerosCounter << 1) < beginCode[onesCounter]) {
        const auto index = beginSymbol[onesCounter - 1] + zerosCounter - beginCode[onesCounter - 1] - 1;
        if (index >= symbolsLength) {
          return;// handle invalid code
        }
        result.emplace_back(otherSymbols[index]);
        onesCounter = 0;
        zerosCounter = 0;
      }
      ++bitCnt;
    });
    ++readBytesCounter;
  });
  std::ranges::transform(result, std::ranges::begin(result), makeRevertLambda<T>(model));
  return result;
}
}// namespace pf::kko

#endif//PF_HUFF_CODEC__DECOMPRESSION_H

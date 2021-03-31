/**
 * @name utils.h
 * @brief utility functions of various kind - minmax, heap, bit manipulation...
 * @author Petr Flajšingr, xflajs00
 * @date 23.03.2021
 */

#ifndef HUFF_CODEC__UTILS_H
#define HUFF_CODEC__UTILS_H

#include <algorithm>
#include <experimental/memory>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <cmath>

namespace std {
template<typename T>
using observer_ptr = experimental::observer_ptr<T>;

template<typename T>
observer_ptr<T> make_observer(T *p) {
  return experimental::make_observer(p);
}
}// namespace std

namespace pf::kko {
/**
 * Constexpr power of for meta programming.
 */
constexpr auto constexpr_pow(std::integral auto base, std::integral auto power) {
  auto result = decltype(base){1};
  while (power--) { result *= base; }
  return result;
}

/**
 * Amount of values which can be represented by T (basically memory bit size.
 */
template<std::integral T>
constexpr auto ValueCount = constexpr_pow(2, sizeof(T) * 8);

template<typename T>
concept RelationComparable = requires(T a, T b) {
  { a < b }
  ->std::convertible_to<bool>;
  { a > b }
  ->std::convertible_to<bool>;
};

/**
 * MinMax implementation working without iterators, which proved to be an issue while using ranges due to borrowed_range requirements.
 * @param range
 * @param comp comparison predicate, returning true if the first value is smaller, false otherwise
 * @return std::nullopt if the range is empty, otherwise std::pair, where first is min and second is max
 */
/* clang-format off */
template<std::ranges::forward_range R, typename F = std::ranges::less>
std::optional<std::pair<std::ranges::range_value_t<R>, std::ranges::range_value_t<R>>>
  minmaxValue(const R &range, F comp = {})
    requires std::is_invocable_r_v<bool, decltype(comp), std::ranges::range_value_t<R>, std::ranges::range_value_t<R>>
            && RelationComparable<std::ranges::range_value_t<R>> {
  if (std::ranges::empty(range)) { return std::nullopt; }
  auto iter = std::ranges::begin(range);
  auto result = std::make_pair(*iter, *iter);
  std::ranges::for_each(range, [&result, comp](const auto &element) {
    if (!comp(result.first, element)) { result.first = element; }
    if (comp(result.second, element)) { result.second = element; }
  });
  return result;
}
/* clang-format on */

/**
 * Inefficient conversion of T to std::vector<bool>, which is its bit representation.
 */
template<typename T>
std::vector<bool> typeToBits(const T &value) {
  auto result = std::vector<bool>{};
  result.reserve(sizeof(T) * 8);
  const auto rawDataPtr = reinterpret_cast<const uint8_t *>(&value);
  for (std::size_t i = 0; i < sizeof(T); ++i) {
    const auto byte = rawDataPtr[i];
    for (int j = 7; j >= 0; --j) { result.template emplace_back(byte & (1 << j)); }
  }
  return result;
}

template<typename T>
std::vector<bool> typeToBits(const T &value, std::size_t bitLength) {
  auto result = std::vector<bool>(bitLength - sizeof(T) * 8, false);
  std::ranges::copy(typeToBits(value), std::back_inserter(result));
  return result;
}

/**
 * Convenience function for heap push.
 * @param range heap data storage
 * @param value value to be added
 * @param compare comparison functions adhering to max-heap rules
 */
void pushAsHeap(std::ranges::random_access_range auto &range, std::ranges::range_value_t<decltype(range)> &&value,
                auto compare = std::less<>()) {
  range.push_back(std::move(value));
  std::push_heap(std::ranges::begin(range), std::ranges::end(range), compare);
}

/**
 * Convenience function for heap pop.
 * @param range heap data storage
 * @param compare comparison functions adhering to max-heap rules
 * @return element at the top of the heap
 */
auto popAsHeap(std::ranges::random_access_range auto &range, auto compare = std::less<>()) {
  std::pop_heap(std::ranges::begin(range), std::ranges::end(range), compare);
  auto result = std::move(range[std::ranges::size(range) - 1]);
  range.pop_back();
  return result;
}

/**
 * Call fnc for each bit of value.
 */
template<typename T>
void forEachBit(const T &value, std::invocable<bool> auto fnc) {
  for (int i = sizeof(T) * 8 - 1; i >= 0; --i) { fnc(value & (T{1} << i)); }
}

/**
 * Call fnc for first n bits of value.
 */
template<typename T>
void forEachBit_n(const T &value, std::size_t n, std::invocable<bool> auto fnc) {
  constexpr auto bitSize = sizeof(T) * 8;
  for (int i = bitSize - 1; i >= static_cast<int>(bitSize - n); --i) { fnc(value & (T{1} << i)); }
}

inline void writeToFile(const std::filesystem::path &path, const std::string &text) {
  auto ostream = std::ofstream(path);
  ostream << text;
}

template <std::integral T>
T binToIntegral(const std::vector<bool> &bin) {
  auto result = T{};
  for (std::size_t i = 0; i < bin.size(); ++i) {
    result += bin[i] ? std::pow(2, 8 - i) : 0;
  }
  return result;
}
}// namespace pf::kko

#endif//HUFF_CODEC__UTILS_H

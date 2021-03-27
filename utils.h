//
// Created by petr on 3/23/21.
//

#ifndef HUFF_CODEC__UTILS_H
#define HUFF_CODEC__UTILS_H

#include <algorithm>
#include <ranges>

constexpr auto constexpr_pow(std::integral auto base, std::integral auto power) {
  auto result = decltype(base){1};
  while (power--) { result *= base; }
  return result;
}

template<std::integral T>
constexpr auto ValueCount = constexpr_pow(2, sizeof(T) * 8);

template<typename T>
concept RelationComparable = requires(T a, T b) {
  { a < b }
  ->std::convertible_to<bool>;
  { a > b }
  ->std::convertible_to<bool>;
};

template<typename T>
struct PriorityQueueMoveHack {
  PriorityQueueMoveHack(T &&value) : value(std::move(value)) {}
  mutable T value;
  bool operator<(const PriorityQueueMoveHack &rhs) const requires std::totally_ordered<T> { return value < rhs.value; }
  bool operator>(const PriorityQueueMoveHack &rhs) const requires std::totally_ordered<T> { return rhs < *this; }
  bool operator<=(const PriorityQueueMoveHack &rhs) const requires std::totally_ordered<T> { return !(rhs < *this); }
  bool operator>=(const PriorityQueueMoveHack &rhs) const requires std::totally_ordered<T> { return !(*this < rhs); }
};

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

void pushAsHeap(std::ranges::random_access_range auto &range, std::ranges::range_value_t<decltype(range)> &&value, auto compare = std::less<>()) {
  range.push_back(std::move(value));
  std::push_heap(std::ranges::begin(range), std::ranges::end(range), compare);
}

auto popAsHeap(std::ranges::random_access_range auto &range, auto compare = std::less<>()) {
  std::pop_heap(std::ranges::begin(range), std::ranges::end(range), compare);
  auto result = std::move(range[std::ranges::size(range) - 1]);
  range.pop_back();
  return result;
}

#endif//HUFF_CODEC__UTILS_H

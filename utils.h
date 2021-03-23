//
// Created by petr on 3/23/21.
//

#ifndef HUFF_CODEC__UTILS_H
#define HUFF_CODEC__UTILS_H

constexpr auto constexpr_pow(std::integral auto base, std::integral auto power) {
  auto result = decltype(base){1};
  while (power--) { result *= base; }
  return result;
}

template<typename T>
struct PriorityQueueMoveHack {
  PriorityQueueMoveHack(T &&value) : value(std::move(value)) {}
  mutable T value;
  bool operator<(const PriorityQueueMoveHack &rhs) const requires std::totally_ordered<T> { return value < rhs.value; }
  bool operator>(const PriorityQueueMoveHack &rhs) const requires std::totally_ordered<T> { return rhs < *this; }
  bool operator<=(const PriorityQueueMoveHack &rhs) const requires std::totally_ordered<T> { return !(rhs < *this); }
  bool operator>=(const PriorityQueueMoveHack &rhs) const requires std::totally_ordered<T> { return !(*this < rhs); }
};

#endif//HUFF_CODEC__UTILS_H

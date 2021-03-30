//
// Created by petr on 3/28/21.
//

#ifndef HUFF_CODEC__ENCODINGTREEDATA_H
#define HUFF_CODEC__ENCODINGTREEDATA_H

#include <iostream>
namespace pf::kko {
template<std::integral T>
struct StaticEncodingTreeData {
  T value;
  std::size_t weight;
  bool isNYT;

  bool operator<(const StaticEncodingTreeData &rhs) const { return weight < rhs.weight; }
  bool operator>(const StaticEncodingTreeData &rhs) const { return rhs.weight < weight; }
  bool operator<=(const StaticEncodingTreeData &rhs) const { return !(rhs < *this); }
  bool operator>=(const StaticEncodingTreeData &rhs) const { return !(*this < rhs); }

  bool operator==(const StaticEncodingTreeData &rhs) const { return weight == rhs.weight; }
  bool operator!=(const StaticEncodingTreeData &rhs) const { return !(rhs == *this); }
};
template<std::integral T>
struct AdaptiveEncodingTreeData {
  T value;
  std::size_t weight;
  bool isNYT;
  std::size_t order;

  bool operator<(const AdaptiveEncodingTreeData &rhs) const { return weight < rhs.weight; }
  bool operator>(const AdaptiveEncodingTreeData &rhs) const { return rhs.weight < weight; }
  bool operator<=(const AdaptiveEncodingTreeData &rhs) const { return !(rhs < *this); }
  bool operator>=(const AdaptiveEncodingTreeData &rhs) const { return !(*this < rhs); }

  bool operator==(const AdaptiveEncodingTreeData &rhs) const { return weight == rhs.weight; }
  bool operator!=(const AdaptiveEncodingTreeData &rhs) const { return !(rhs == *this); }
};

template <std::integral T>
StaticEncodingTreeData<T> makeNYTStatic() {
  return StaticEncodingTreeData<T>{T{}, 0, true};
}
template <std::integral T>
AdaptiveEncodingTreeData<T> makeNYTAdaptive(std::size_t order = 512/*std::numeric_limits<std::size_t>::max()*/) {
  return AdaptiveEncodingTreeData<T>{T{}, 0, true, order};
}

}// namespace pf::kko
#endif//HUFF_CODEC__ENCODINGTREEDATA_H

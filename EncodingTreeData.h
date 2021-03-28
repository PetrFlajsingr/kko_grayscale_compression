//
// Created by petr on 3/28/21.
//

#ifndef HUFF_CODEC__ENCODINGTREEDATA_H
#define HUFF_CODEC__ENCODINGTREEDATA_H

template<std::integral T>
struct EncodingTreeData {
  T value;
  std::size_t weight;
  bool isValid;

  bool operator<(const EncodingTreeData &rhs) const { return weight < rhs.weight; }
  bool operator>(const EncodingTreeData &rhs) const { return rhs.weight < weight; }
  bool operator<=(const EncodingTreeData &rhs) const { return !(rhs < *this); }
  bool operator>=(const EncodingTreeData &rhs) const { return !(*this < rhs); }

  bool operator==(const EncodingTreeData &rhs) const { return weight == rhs.weight; }
  bool operator!=(const EncodingTreeData &rhs) const { return !(rhs == *this); }
};

#endif//HUFF_CODEC__ENCODINGTREEDATA_H

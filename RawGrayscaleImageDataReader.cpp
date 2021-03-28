//
// Created by petr on 3/23/21.
//

#include "RawGrayscaleImageDataReader.h"
#include <algorithm>

namespace pf::kko {
RawGrayscaleImageDataReader::RawGrayscaleImageDataReader(const std::filesystem::path &src, size_t imageWidth)
    : input(src, std::ios::binary | std::ios::in), width(imageWidth) {}

std::vector<uint8_t> RawGrayscaleImageDataReader::readAllRaw() {
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::vector<std::vector<uint8_t>> RawGrayscaleImageDataReader::readAll2D() {
  const auto rawData = readAllRaw();
  auto result = std::vector<std::vector<uint8_t>>{};
  result.reserve(rawData.size() / width);
  auto srcIter = rawData.cbegin();
  const auto srcEnd = rawData.cend();
  while (srcIter < srcEnd) {
    auto &row = result.emplace_back();
    row.reserve(width);
    std::ranges::copy_n(srcIter, width, std::back_inserter(row));
    srcIter += width;
  }
  return result;
}

RawGrayscaleImageDataReader::iterator RawGrayscaleImageDataReader::begin() { return iterator{*this}; }
RawGrayscaleImageDataReader::sentinel RawGrayscaleImageDataReader::end() { return sentinel{}; }

RawGrayscaleImageDataReader::Iterator::Iterator(RawGrayscaleImageDataReader &reader) : fileIter(reader.input) {}

RawGrayscaleImageDataReader::value_type RawGrayscaleImageDataReader::Iterator::operator*() const { return *fileIter; }
RawGrayscaleImageDataReader::Iterator RawGrayscaleImageDataReader::Iterator::operator++(int) {
  auto copy = *this;
  operator++();
  return copy;
}
RawGrayscaleImageDataReader::Iterator &RawGrayscaleImageDataReader::Iterator::operator++() {
  ++fileIter;
  return *this;
}
bool RawGrayscaleImageDataReader::Iterator::operator==(const RawGrayscaleImageDataReader::Sentinel &) const {
  return fileIter == std::istreambuf_iterator<char>{};
}

}// namespace pf::kko
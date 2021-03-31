//
// Created by petr on 3/23/21.
//

#ifndef PF_HUFF_CODEC__RAWGRAYSCALEIMAGEDATAREADER_H
#define PF_HUFF_CODEC__RAWGRAYSCALEIMAGEDATAREADER_H

#include <algorithm>
#include <concepts>
#include <filesystem>
#include <fstream>
#include <vector>

namespace pf::kko {

class RawGrayscaleImageDataReader {
 public:
  class Sentinel {
   public:
    Sentinel() = default;
    Sentinel(const Sentinel &other) = default;
    Sentinel &operator=(const Sentinel &other) = default;
    Sentinel(Sentinel &&other) = default;
    Sentinel &operator=(Sentinel &&other) = default;
  };
  class Iterator {
   public:
    using difference_type = int;
    using value_type = uint8_t;
    using reference = uint8_t &;
    Iterator() = default;
    explicit inline Iterator(RawGrayscaleImageDataReader &reader) : fileIter(reader.input) {}
    Iterator(const Iterator &other) = default;
    Iterator &operator=(const Iterator &other) = default;
    Iterator(Iterator &&other) = default;
    Iterator &operator=(Iterator &&other) = default;

    [[nodiscard]] inline value_type operator*() const { return *fileIter; }
    inline Iterator operator++(int) {
      auto copy = *this;
      operator++();
      return copy;
    }
    inline Iterator &operator++() {
      ++fileIter;
      return *this;
    }

    [[nodiscard]] inline bool operator==(const Sentinel &) const {
      return fileIter == std::istreambuf_iterator<char>{};
    }

   private:
    std::istreambuf_iterator<char> fileIter;
  };

  using value_type = uint8_t;
  using pointer = uint8_t *;
  using const_pointer = const pointer;
  using reference = uint8_t &;
  using const_reference = const uint8_t &;
  using iterator = Iterator;
  using sentinel = Sentinel;
  /**
   *
   * @param src path to source file
   * @param imageWidth
   */
  inline explicit RawGrayscaleImageDataReader(const std::filesystem::path &src, size_t imageWidth)
      : input(src, std::ios::binary | std::ios::in), width(imageWidth) {}

  /**
   * @return whole file as array of bytes
   */
  [[nodiscard]] inline std::vector<uint8_t> readAllRaw() {
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
  }
  /**
   * @return whole file as a 2D array, where each row is of 'imageWidth' length
   */
  [[nodiscard]] inline std::vector<std::vector<uint8_t>> readAll2D() {
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

  [[nodiscard]] inline iterator begin() { return iterator{*this}; }
  [[nodiscard]] inline sentinel end() { return sentinel{}; }

 private:
  std::ifstream input;
  std::size_t width;
};

static_assert(std::input_iterator<RawGrayscaleImageDataReader::Iterator>);
static_assert(std::sentinel_for<RawGrayscaleImageDataReader::Sentinel, RawGrayscaleImageDataReader::Iterator>);
}// namespace pf::kko

#endif//KKO_GRAYSCALE_COMPRESSION__RAWGRAYSCALEIMAGEDATAREADER_H

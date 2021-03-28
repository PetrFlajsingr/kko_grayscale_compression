//
// Created by petr on 3/23/21.
//

#ifndef PF_HUFF_CODEC__RAWGRAYSCALEIMAGEDATAREADER_H
#define PF_HUFF_CODEC__RAWGRAYSCALEIMAGEDATAREADER_H

#include <concepts>
#include <filesystem>
#include <fstream>
#include <vector>

namespace pf::kko {
/**
 * Pro cteni raw grayscale obrazku, pripadne postupnou iteraci bez nacteni vsech dat do pameti
 */
class RawGrayscaleImageDataReader {
 public:
  class Iterator;
  class Sentinel;

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
  explicit RawGrayscaleImageDataReader(const std::filesystem::path &src, size_t imageWidth);

  /**
   * @return whole file as array of bytes
   */
  [[nodiscard]] std::vector<uint8_t> readAllRaw();
  /**
   * @return whole file as a 2D array, where each row is of 'imageWidth' length
   */
  [[nodiscard]] std::vector<std::vector<uint8_t>> readAll2D();

  [[nodiscard]] iterator begin();
  [[nodiscard]] sentinel end();

 private:
  std::ifstream input;
  std::size_t width;
};

class RawGrayscaleImageDataReader::Iterator {
 public:
  using difference_type = int;
  using value_type = uint8_t;
  using reference = uint8_t &;
  Iterator() = default;
  explicit Iterator(RawGrayscaleImageDataReader &reader);
  Iterator(const Iterator &other) = default;
  Iterator &operator=(const Iterator &other) = default;
  Iterator(Iterator &&other) = default;
  Iterator &operator=(Iterator &&other) = default;

  [[nodiscard]] value_type operator*() const;
  Iterator operator++(int);
  Iterator &operator++();

  [[nodiscard]] bool operator==(const Sentinel &) const;

 private:
  std::istreambuf_iterator<char> fileIter;
};
class RawGrayscaleImageDataReader::Sentinel {
 public:
  Sentinel() = default;
  Sentinel(const Sentinel &other) = default;
  Sentinel &operator=(const Sentinel &other) = default;
  Sentinel(Sentinel &&other) = default;
  Sentinel &operator=(Sentinel &&other) = default;
};

static_assert(std::input_iterator<RawGrayscaleImageDataReader::Iterator>);
static_assert(std::sentinel_for<RawGrayscaleImageDataReader::Sentinel, RawGrayscaleImageDataReader::Iterator>);
}// namespace pf::kko

#endif//KKO_GRAYSCALE_COMPRESSION__RAWGRAYSCALEIMAGEDATAREADER_H

//
// Created by petr on 3/23/21.
//

#ifndef KKO_GRAYSCALE_COMPRESSION__RAWGRAYSCALEIMAGEDATAREADER_H
#define KKO_GRAYSCALE_COMPRESSION__RAWGRAYSCALEIMAGEDATAREADER_H

#include <concepts>
#include <filesystem>
#include <fstream>
#include <vector>

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
   * @param src cesta ke zdrojovemu souboru
   * @param imageWidth sirka obrazku
   */
  explicit RawGrayscaleImageDataReader(const std::filesystem::path &src, int32_t imageWidth);

  /**
   * @return cely soubor jako 1D pole bytu
   */
  [[nodiscard]] std::vector<uint8_t> readAllRaw();
  /**
   * @return cely soubor jako 2D pole bytu, kdy kazdy 'radek' ma delku width
   */
  [[nodiscard]] std::vector<std::vector<uint8_t>> readAll2D();

  [[nodiscard]] iterator begin();
  [[nodiscard]] sentinel end();

 private:
  std::ifstream input;
  int32_t width;
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

 private:
  std::istreambuf_iterator<char> sentinelIter{};
};

static_assert(std::input_iterator<RawGrayscaleImageDataReader::Iterator>);
static_assert(std::sentinel_for<RawGrayscaleImageDataReader::Sentinel, RawGrayscaleImageDataReader::Iterator>);

#endif//KKO_GRAYSCALE_COMPRESSION__RAWGRAYSCALEIMAGEDATAREADER_H

//
// Created by petr on 3/26/21.
//

#ifndef HUFF_CODEC__BINARYENCODER_H
#define HUFF_CODEC__BINARYENCODER_H

#include "utils.h"
#include <concepts>
#include <spdlog/spdlog.h>
#include <vector>

/**
 * Prevod ruznych reprezentaci dat v cpp na binarni data.
 * Neni prilis efektivni, i kdyz pri aktivaci optimalizaci se rychlost zvysi az o 2 rady
 */
template<std::integral T = uint8_t>
class BinaryEncoder {
 public:
  using size_type = std::size_t;

 private:
  class BitAccessor {
   public:
    BitAccessor(BinaryEncoder &parent, const size_type bitIndex) : parent(parent), bitIndex(bitIndex) {}
    BitAccessor &operator=(bool newValue) {
      parent.setBitValue(bitIndex, newValue);
      return *this;
    }
    operator bool() const { return parent.getBitValue(bitIndex); }

   private:
    BinaryEncoder &parent;
    const size_type bitIndex{};
  };

 public:
  BinaryEncoder() = default;
  /**
   *
   * @param initSize pocatecni pocet nastavenych bitu
   * @param initValue hodnota pro pocatecni nastaveni bitu
   */
  explicit BinaryEncoder(size_type initSize, bool initValue) : size_(initSize) {
    if (initSize != 0) {
      const auto val = initValue ? ONES : ZEROS;
      rawData.resize(sizeForBits(initSize), val);
    }
  }

  /**
   *
   * @return pocet aktivne vyuzivanych bitu
   */
  [[nodiscard]] size_type size() const { return size_; }
  void resize(size_type newSize, bool initValue) {
    const auto val = initValue ? ONES : ZEROS;
    rawData.resize(sizeForBits(newSize), val);
    size_ = newSize;
  }

  /**
   *
   * @return pocet alokovanych bitu
   */
  [[nodiscard]] size_type capacity() const { return rawData.capacity() * TYPE_BIT_SIZE; }

  /**
   * Alokace pameti
   * @param newCapacity pozadovane mnozstvi alokovane pameti v bitech
   */
  void reserve(size_type newCapacity) { rawData.reserve(sizeForBits(newCapacity)); }

  /**
   * Pristup k hodnote bitu pomoci proxy objektu.
   * @param bitIndex index bitu
   * @return Proxy objekt umoznujici cteni prevodem na bool a nastaveni nove hodnoty pomoci operator=
   */
  [[nodiscard]] BitAccessor operator[](size_type bitIndex) { return BitAccessor(*this, bitIndex); }

  /**
   * Cteni hodnoty bitu na indexu
   * @param bitIndex
   * @return hodnota bitu na indexu
   */
  [[nodiscard]] bool operator[](size_type bitIndex) const { return getBitValue(bitIndex); }
  /**
   * Cteni hodnoty bitu na indexu
   * @param bitIndex
   * @return hodnota bitu na indexu
   */
  [[nodiscard]] bool getBitValue(size_type bitIndex) const {
    const auto [index, mask] = indexAndMaskForBit(bitIndex);
    return rawData[index] & mask;
  }

  /**
   * Nastaveni hodnoty bitu na indexu
   * @param bitIndex index bitu
   * @param value nova hodnota
   */
  void setBitValue(size_type bitIndex, bool value) {
    if (value) {
      setBit(bitIndex);
    } else {
      resetBit(bitIndex);
    }
  }

  void pushBack(const std::vector<bool> &data) {
    for (const auto bit : data) { pushBack(bit); }
  }

  void pushBack(bool bit) {
    checkAndIncreaseCapacity();
    setBitValue(size(), bit);
    ++size_;
  }

  template<typename U>
  void pushBack(const U &value) {
    if constexpr (std::ranges::range<U>) {
      std::ranges::for_each(value, [this](auto el) { pushBack(typeToBits(el)); });
    } else {
      pushBack(typeToBits(value));
    }
  }

  void shrinkToFit() { rawData.shrink_to_fit(); }

  void addPadding() {
    for (auto i = 0; i < unusedBitsInCell(); ++i) { pushBack(false); }
  }

  [[nodiscard]] size_type unusedBitsInCell() const { return 8 - size() % TYPE_BIT_SIZE; }

  [[nodiscard]] const std::vector<T> &data() const { return rawData; }

  [[nodiscard]] std::vector<T> releaseData() { return std::move(rawData); }

 private:
  [[nodiscard]] constexpr size_type sizeForBits(size_type bits) const {
    const auto div = bits / TYPE_BIT_SIZE;
    const auto mod = bits % TYPE_BIT_SIZE;
    return div + (mod != 0 ? 1 : 0);
  }

  [[nodiscard]] constexpr std::pair<size_type, T> indexAndMaskForBit(size_type bitIndex) const {
    const auto index = bitIndex / TYPE_BIT_SIZE;
    const auto remainder = 7 - (bitIndex - index * TYPE_BIT_SIZE);
    const auto mask = T{1} << remainder;
    return {index, mask};
  }

  void setBit(size_type bitIndex) {
    const auto [index, mask] = indexAndMaskForBit(bitIndex);
    rawData[index] |= mask;
  }
  void resetBit(size_type bitIndex) {
    const auto [index, mask] = indexAndMaskForBit(bitIndex);
    rawData[index] &= (~mask);
  }

  void checkAndIncreaseCapacity() {
    const auto vectorSize = rawData.size() * 8;
    if (size() == vectorSize) { rawData.template emplace_back(); }
  }

  constexpr static T ONES = ~T{0};
  constexpr static T ZEROS = T{0};
  constexpr static size_type TYPE_BIT_SIZE = sizeof(T) * 8;
  size_type size_{};
  std::vector<T> rawData{};
};

#endif//HUFF_CODEC__BINARYENCODER_H

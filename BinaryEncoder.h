/**
 * @name BinaryEncoding.h
 * @brief class for writing binary data to memory - simplified serialisation etc.
 * @author Petr Flajšingr, xflajs00
 * @date 26.03.2021
 */
#ifndef HUFF_CODEC__BINARYENCODER_H
#define HUFF_CODEC__BINARYENCODER_H

#include "utils.h"
#include <concepts>
#include <spdlog/spdlog.h>
#include <vector>

namespace pf::kko {
/**
 * Conversion of various data types to binary code.
 * Not very efficient without the use of compiler optimisations.
 */
template<std::integral T = uint8_t>
class BinaryEncoder {
 public:
  using size_type = std::size_t;

 private:
  /**
   * Proxy class for operator[] bit value setting
   */
  class BitAccessor {
   public:
    BitAccessor(BinaryEncoder &parent, const size_type bitIndex) : parent(parent), bitIndex(bitIndex) {}
    BitAccessor &operator=(bool newValue) {
      parent.setBitValue(bitIndex, newValue);
      return *this;
    }
    /**
     * Implicit conversion so that bool a = binEncoder[x] works
     * @return boolean value of the bit represented by this object
     */
    operator bool() const { return parent.getBitValue(bitIndex); }

   private:
    BinaryEncoder &parent;
    const size_type bitIndex{};
  };

 public:
  BinaryEncoder() = default;
  /**
   *
   * @param initSize init size of memory in bits
   * @param initValue init value for memory allocated in ctor
   */
  explicit BinaryEncoder(size_type initSize, bool initValue) : size_(initSize) {
    if (initSize != 0) {
      const auto val = initValue ? ONES : ZEROS;
      rawData.resize(sizeForBits(initSize), val);
    }
  }

  /**
   *
   * @return amount of bits currently in use
   */
  [[nodiscard]] size_type size() const { return size_; }
  void resize(size_type newSize, bool initValue) {
    const auto val = initValue ? ONES : ZEROS;
    rawData.resize(sizeForBits(newSize), val);
    size_ = newSize;
  }

  /**
   *
   * @return amount of bits which are currently reserved in memory
   */
  [[nodiscard]] size_type capacity() const { return rawData.capacity() * TYPE_BIT_SIZE; }

  /**
   *
   * @param newCapacity desired bit count to be allocated
   */
  void reserve(size_type newCapacity) { rawData.reserve(sizeForBits(newCapacity)); }

  /**
   * Access to bit value via a proxy object
   * @param bitIndex
   * @return @see BitAccessor
   */
  [[nodiscard]] BitAccessor operator[](size_type bitIndex) { return BitAccessor(*this, bitIndex); }

  /**
   *
   * @param bitIndex
   * @return value of bit on index
   */
  [[nodiscard]] bool operator[](size_type bitIndex) const { return getBitValue(bitIndex); }

  /**
   *
   * @param bitIndex
   * @return value of bit on index
   */
  [[nodiscard]] bool getBitValue(size_type bitIndex) const {
    const auto [index, mask] = indexAndMaskForBit(bitIndex);
    return rawData[index] & mask;
  }

  /**
   *
   * @param bitIndex
   * @param value value to be set
   */
  void setBitValue(size_type bitIndex, bool value) {
    if (value) {
      setBit(bitIndex);
    } else {
      resetBit(bitIndex);
    }
  }

  /**
   * Conversion of vector<bool> to binary data. Considers every element a single bit.
   * @param data
   */
  void pushBack(const std::vector<bool> &data) {
    for (const auto bit : data) { pushBack(bit); }
  }

  /**
   * Pushes single bit to the end.
   * @param bit value to be pushed
   */
  void pushBack(bool bit) {
    checkAndIncreaseCapacity();
    setBitValue(size(), bit);
    ++size_;
  }

  /**
   * Allows for direct conversion of types to bits. Not very efficient.
   * @tparam U
   * @param value value to be converted
   */


  template <typename ...Args>
  void pushBack(const Args &...values) {
    (pushBack_impl(values), ...);
  }

  /**
   * Shrink capacity to be the same as size (aligned to sizeof(T)).
   */
  void shrinkToFit() { rawData.shrink_to_fit(); }

  /**
   * Add zero padding to the end of the data, so that size is aligned to sizeof(T)
   */
  void addPadding() {
    for (auto i = 0; i < unusedBitsInCell(); ++i) { pushBack(false); }
  }

  /**
   *
   * @return amount of bits which are currently unused in sizeof(T) B
   */
  [[nodiscard]] size_type unusedBitsInCell() const { return 8 - size() % TYPE_BIT_SIZE; }

  /**
   * Direct read-only access to underlying data structure
   */
  [[nodiscard]] const std::vector<T> &data() const { return rawData; }

  /**
   * Move data out of the object.
   * @return vector of binary data
   */
  [[nodiscard]] std::vector<T> releaseData() { return std::move(rawData); }

 private:
  template<typename U>
  void pushBack_impl(const U &value) {
    if constexpr (std::ranges::range<U>) {
      std::ranges::for_each(value, [this](auto el) { pushBack(typeToBits(el)); });
    } else {
      pushBack(typeToBits(value));
    }
  }

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
}// namespace pf::kko

#endif//HUFF_CODEC__BINARYENCODER_H

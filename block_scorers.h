/**
 * @name block_scorers.h
 * @brief structures for block traversal scoring
 * @author Petr Flajšingr, xflajs00
 * @date 6.04.2021
 */

#ifndef HUFF_CODEC__BLOCK_SCORERS_H
#define HUFF_CODEC__BLOCK_SCORERS_H

namespace pf::kko {

/**
 * Description of requirements for object scoring block traversal suitability.
 */
template<typename T, typename ValueType>
concept BlockScorer = requires(T t, ValueType val) {
  {t.next(val)};
  { t.getScore() }
  ->std::convertible_to<int>;
  {t.reset()};
  { T::MaxScore }
  ->std::convertible_to<int>;
};

/**
 * Scoring data via similarity of neighboring values;
 */
struct NeighborDifferenceScorer {
  inline void next(uint8_t value) {
    score -= std::abs(static_cast<int>(value) - lastVal);
    lastVal = value;
  }

  [[nodiscard]] inline int getScore() const { return score; }

  inline void reset() {
    lastVal = 0;
    score = MaxScore;
  }
  static constexpr auto MaxScore = std::numeric_limits<int>::max();
  int lastVal{};
  int score{};
};

/**
 * Scoring data via count of same neighboring values.
 */
struct SameNeighborsScorer {
  inline void next(uint8_t value) {
    score -= value == lastVal ? 0 : 1;
    lastVal = value;
  }

  [[nodiscard]] inline int getScore() const { return score; }

  inline void reset() {
    lastVal = 0;
    score = MaxScore;
  }
  static constexpr auto MaxScore = std::numeric_limits<int>::max();
  int lastVal{};
  int score{};
};
}// namespace pf::kko

#endif//HUFF_CODEC__BLOCK_SCORERS_H

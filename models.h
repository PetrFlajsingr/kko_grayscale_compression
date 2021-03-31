/**
 * @name models.h
 * @brief concepts and models for data transformation while encoding/decoding
 * @author Petr Flajšingr, xflajs00
 * @date 27.03.2021
 */

#ifndef HUFF_CODEC__MODELS_H
#define HUFF_CODEC__MODELS_H

#include <concepts>
#include <utility>

namespace pf::kko {
template<typename T, typename ValueType>
concept Model = requires(T model, ValueType value) {
  { model.apply(value) }
  ->std::same_as<ValueType>;
  { model.revert(value) }
  ->std::same_as<ValueType>;
};

template<typename T>
struct IdentityModel {
  [[nodiscard]] T apply(const T &value) { return value; }
  [[nodiscard]] T revert(const T &value) { return value; }
};

template<typename T>
class NeighborDifferenceModel {
 public:
  [[nodiscard]] T apply(const T &value) {
    const auto result = value - lastVal;
    lastVal = value;
    return result;
  }
  [[nodiscard]] T revert(const T &value) {
    const auto result = value + lastVal;
    lastVal = result;
    return result;
  }

 private:
  T lastVal{};
};

static_assert(Model<IdentityModel<int>, int>);
static_assert(Model<NeighborDifferenceModel<int>, int>);

template<typename T>
auto makeApplyLambda(Model<T> auto &&model) {
  return [m = std::forward<decltype(model)>(model)](auto &value) mutable { return m.apply(value); };
}

template<typename T>
auto makeRevertLambda(Model<T> auto &&model) {
  return [m = std::forward<decltype(model)>(model)](auto &value) mutable { return m.revert(value); };
}
}// namespace pf::kko

#endif//HUFF_CODEC__MODELS_H

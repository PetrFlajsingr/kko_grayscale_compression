/**
 * @name constants.h
 * @brief global space for constants
 * @author Petr Flajšingr, xflajs00
 * @date 28.03.2021
 */
#ifndef HUFF_CODEC__CONSTANTS_H
#define HUFF_CODEC__CONSTANTS_H

namespace pf::kko {
constexpr auto PADDING_MASK = 0b11100000;
constexpr auto PADDING_SHIFT = 5;

using Dimensions = std::pair<std::size_t, std::size_t>;

}// namespace pf::kko

#endif//HUFF_CODEC__CONSTANTS_H

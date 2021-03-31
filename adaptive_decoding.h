//
// Created by petr on 3/31/21.
//

#ifndef HUFF_CODEC__ADAPTIVE_DECODING_H
#define HUFF_CODEC__ADAPTIVE_DECODING_H

#include "adaptive_common.h"
#include "models.h"
#include <concepts>
#include <ranges>
#include <tl/expected.hpp>

namespace pf::kko {
template<std::integral T, typename Model = IdentityModel<T>>
tl::expected<std::vector<T>, std::string> decodeAdaptive(std::ranges::forward_range auto &&data,
                                                         Model &&model = IdentityModel<T>()) {
  enum class State { Tree, Value };
  auto tree = detail::TreeType<T>{};
  tree.setRoot(makeUniqueNode(makeNYTAdaptive<T>()));

  auto symbolNodes = detail::NodeCacheArray<T>{nullptr};

  auto node = std::make_observer(&tree.getRoot());
  auto nytNode = node;

  auto result = std::vector<T>{};

  auto inspectedCode = std::vector<bool>{};
  auto state = State::Value;
  bool done = false;
  std::ranges::for_each(data, [&](const auto byte) {
    if (done) { return; }
    forEachBit(byte, [&](bool bit) {
      if (done) { return; }
      switch (state) {
        case State::Tree: {
          if (bit) {
            node = std::make_observer(&node->getRight());
          } else {
            node = std::make_observer(&node->getLeft());
          }
          break;
        }
        case State::Value: {
          break;
        }
      }
      auto symbol = std::optional<T>{};
      if (node->isLeaf() && !(*node)->isNYT) {
        symbol = (*node)->value;
      } else if ((*node)->isNYT) {
        if (state == State::Value) { inspectedCode.template emplace_back(bit); }
        state = State::Value;
        if (inspectedCode.size() == 9) {
          symbol = binToIntegral<T>(inspectedCode);
          inspectedCode.clear();
          state = State::Tree;
        }
      }

      if (symbol.has_value()) {
        if (*symbol == PSEUDO_EOF<T>) {
          done = true;
          return;
        }
        nytNode = updateTree(tree, *symbol, nytNode, symbolNodes);
        node = std::make_observer(&tree.getRoot());
        result.emplace_back(*symbol);
      }
    });
  });
  [[maybe_unused]] auto a = std::string(result.begin(), result.end());

  std::ranges::transform(result, std::ranges::begin(result), makeRevertLambda<T>(model));
  return result;
}
}// namespace pf::kko

#endif//HUFF_CODEC__ADAPTIVE_DECODING_H

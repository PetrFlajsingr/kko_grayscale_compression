/**
 * @name adaptive_blocks_decoding.h
 * @brief decoding of data encoded using adaptive huffman encoding and adaptive image scanning
 * @author Petr Flajšingr, xflajs00
 * @date 3.4.2021
 */

#ifndef HUFF_CODEC__ADAPTIVE_BLOCKS_DECODING_H
#define HUFF_CODEC__ADAPTIVE_BLOCKS_DECODING_H

#include "AdaptiveImageScanner.h"
#include "adaptive_common.h"
#include <fmt/core.h>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

namespace pf::kko {

struct ImageHeader {
  uint16_t width{};
  uint16_t height{};
  uint8_t blockWidth{};
  uint8_t blockHeight{};
};

struct ImageHeaderBuilder {
  ImageHeader header{};
  std::size_t bytesRead{};
  void add(uint8_t byte) {
    if (bytesRead == 0) { header.width = byte; }
    if (bytesRead == 1) { header.width |= byte << 8; }
    if (bytesRead == 2) { header.height = byte; }
    if (bytesRead == 3) { header.height |= byte << 8; }
    if (bytesRead == 4) { header.blockWidth = byte; }
    if (bytesRead == 5) { header.blockHeight = byte; }
    ++bytesRead;
  }

  [[nodiscard]] bool isDone() { return bytesRead == 6; }
};

struct BlockHeader {
  ScanMethod scanMethod;
  uint8_t bitsRead{};
};

/**
 * Structure for calculating coordinates of decoded symbols.
 */
struct BlockScanData {
  std::size_t blockIndex = 0;
  std::size_t index;
  std::pair<std::size_t, std::size_t> pos;
  ZigZagState zigZagState;
  Dimensions blockSize;
  ScanMethod scanMethod;
  std::size_t imageWidth;

  inline void move() {
    ++index;
    switch (scanMethod) {
      case ScanMethod::Vertical: pos = vertical(index, blockSize); break;
      case ScanMethod::Horizontal: pos = horizontal(index, blockSize); break;
      case ScanMethod::ZigZag:
        const auto [newState, newPos] = zigZagMove(pos, zigZagState, blockSize.first, blockSize.second);
        zigZagState = newState;
        pos = newPos;
        break;
    }
  }

  inline Dimensions getPosInData() {
    auto result = startPosForBlock(blockIndex, imageWidth, blockSize);
    result.first += pos.first;
    result.second += pos.second;
    return result;
  }

  inline void reset(ScanMethod method) {
    index = 0;
    zigZagState = ZigZagState::Right;
    pos = {0, 0};
    scanMethod = method;
  }
};

enum class DecodeState { ImageHeader, BlockHeader, Tree, Value };

template<typename T, Model<T> Model>
struct DecodeContext {
  void setHeader(ImageHeader newHeader) {
    header = newHeader;
    symbolsInBlock = header.blockHeight * header.blockWidth;
    blockScanData.imageWidth = header.width;
    blockScanData.blockSize = {header.blockWidth, header.blockHeight};
  }
  ImageHeader header{};
  BlockHeader blockHeader{};
  std::size_t symbolsInBlock{};
  BlockScanData blockScanData{};
  DecodeState state = DecodeState::ImageHeader;

  uint8_t scanMethodBuffer{};
  uint16_t symbolCounter{};
  Model currentlyUsedModel{};

  bool isFirstBlock = true;
};

/**
 * decode data encoded using adaptive huffman encoding and adaptive image scanning
 * @param data data encoded using adaptive huffman encoding and adaptive image scanni
 * @param model transformation of neighboring data
 * @return decoded data
 */
template<std::integral T>
tl::expected<std::vector<T>, std::string> decodeImageAdaptiveBlocks(std::ranges::forward_range auto &&data,
                                                                    Model<T> auto &&model) {
  auto tree = detail::TreeType<T>{};
  tree.setRoot(makeUniqueNode(makeNYTAdaptive<T>()));

  auto symbolNodes = detail::NodeCacheArray<T>{nullptr};

  auto node = std::make_observer(&tree.getRoot());
  auto nytNode = node;
  [[maybe_unused]] const auto resetTree = [&] {
    std::ranges::fill(symbolNodes, nullptr);
    tree.setRoot(makeUniqueNode(makeNYTAdaptive<T>()));
    node = std::make_observer(&tree.getRoot());
    nytNode = node;
  };

  auto result = std::vector<T>{};
  auto resultView = View2D<std::vector<T>, false>{};

  auto inspectedCode = std::vector<bool>{};
  bool done = false;

  auto headerBuilder = ImageHeaderBuilder{};
  auto ctx = DecodeContext<T, std::decay_t<decltype(model)>>{};
  std::ranges::for_each(data, [&](const auto byte) {
    switch (ctx.state) {
      case DecodeState::ImageHeader:
        headerBuilder.add(byte);
        if (headerBuilder.isDone()) {
          ctx.setHeader(headerBuilder.header);
          ctx.state = DecodeState::BlockHeader;
          result.resize(ctx.header.width * ctx.header.height);
          resultView = makeView2D(result, ctx.header.width);
        }
        return;
      default: break;
    }

    if (done) { return; }
    forEachBit(byte, [&](bool bit) {
      if (done) { return; }
      switch (ctx.state) {
        case DecodeState::ImageHeader: throw std::runtime_error("Invalid state");
        case DecodeState::BlockHeader:
          ++ctx.blockHeader.bitsRead;
          ctx.scanMethodBuffer |= ((bit ? 1 : 0) << (3 - ctx.blockHeader.bitsRead));
          if (ctx.blockHeader.bitsRead == 3) {
            if (ctx.scanMethodBuffer == 0b111) {
              done = true;
              return;
            }
            ctx.blockHeader.scanMethod = static_cast<ScanMethod>(ctx.scanMethodBuffer);
            ctx.blockScanData.reset(ctx.blockHeader.scanMethod);
            ctx.state = ctx.isFirstBlock ? DecodeState::Value : DecodeState::Tree;
            //ctx.state = DecodeState::Value;
            //resetTree();
            ctx.isFirstBlock = false;
            ctx.currentlyUsedModel = model;
            ctx.blockHeader.bitsRead = 0;
          }
          return;
        case DecodeState::Tree: {
          if (bit) {
            node = std::make_observer(&node->getRight());
          } else {
            node = std::make_observer(&node->getLeft());
          }
          break;
        }
        case DecodeState::Value: {
          break;
        }
      }
      auto symbol = std::optional<T>{};
      if (node->isLeaf() && !(*node)->isNYT) {
        symbol = (*node)->value;
      } else if ((*node)->isNYT) {
        if (ctx.state == DecodeState::Value) { inspectedCode.template emplace_back(bit); }
        ctx.state = DecodeState::Value;
        if (inspectedCode.size() == 9) {
          symbol = binToIntegral<T>(inspectedCode);
          inspectedCode.clear();
          ctx.state = DecodeState::Tree;
        }
      }

      if (symbol.has_value()) {
        nytNode = updateTree(tree, *symbol, nytNode, symbolNodes);
        node = std::make_observer(&tree.getRoot());

        const auto pos = ctx.blockScanData.getPosInData();
        if (pos.first < ctx.header.width && pos.second < ctx.header.height) {
          resultView[pos.first][pos.second] = ctx.currentlyUsedModel.revert(*symbol);
        }
        ctx.blockScanData.move();
        ++ctx.symbolCounter;
        if (ctx.symbolCounter == ctx.symbolsInBlock) {
          ctx.symbolCounter = 0;
          ctx.state = DecodeState::BlockHeader;
          ctx.scanMethodBuffer = 0;
          ++ctx.blockScanData.blockIndex;
        }
      }
    });
  });

  return result;
}

}// namespace pf::kko

#endif//HUFF_CODEC__ADAPTIVE_BLOCKS_DECODING_H

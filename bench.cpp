/**
 * @name bench.cpp
 * @brief benchmark and result validation
 * @author Petr Flajšingr, xflajs00
 * @date 28.03.2021
 */

#ifndef HUFF_CODEC__BENCH_H
#define HUFF_CODEC__BENCH_H

#define FMT_HEADER_ONLY
#include "RawGrayscaleImageDataReader.h"
#include "Tree.h"
#include "adaptive_blocks_decoding.h"
#include "adaptive_blocks_encoding.h"
#include "adaptive_decoding.h"
#include "adaptive_encoding.h"
#include "argparse.hpp"
#include "args/ValidPathCheckAction.h"
#include "fmt/core.h"
#include "fmt/ostream.h"
#include "magic_enum.hpp"
#include "spdlog/spdlog.h"
#include "static_decoding.h"
#include "static_encoding.h"
#include <optional>
#include <span>
#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"
using namespace pf::kko;

constexpr auto IMAGE_WIDTH = 512;

std::function<std::vector<uint8_t>(std::vector<uint8_t> &&)> getEncodeFnc(bool enableStatic, bool enableModel,
                                                                          bool adaptive) {
  if (enableStatic) {
    if (enableModel) {
      return [](auto &&data) {
        return pf::kko::encodeStatic<uint8_t>(std::move(data), pf::kko::NeighborDifferenceModel<uint8_t>{});
      };
    } else {
      return [](auto &&data) {
        return pf::kko::encodeStatic<uint8_t>(std::move(data), pf::kko::IdentityModel<uint8_t>{});
      };
    }
  }
  if (!adaptive) {
    if (enableModel) {
      return [](auto &&data) {
        return pf::kko::encodeAdaptive<uint8_t>(std::move(data), pf::kko::NeighborDifferenceModel<uint8_t>{});
      };
    } else {
      return [](auto &&data) {
        return pf::kko::encodeAdaptive<uint8_t>(std::move(data), pf::kko::IdentityModel<uint8_t>{});
      };
    }
  } else {
    const auto imgWidth = IMAGE_WIDTH;
    if (enableModel) {
      return [imgWidth](auto &&data) {
        return pf::kko::encodeImageAdaptiveBlocks<uint8_t>(std::move(data), imgWidth,
                                                           pf::kko::NeighborDifferenceModel<uint8_t>{});
      };
    } else {
      return [imgWidth](auto &&data) {
        return pf::kko::encodeImageAdaptiveBlocks<uint8_t>(std::move(data), imgWidth,
                                                           pf::kko::IdentityModel<uint8_t>{});
      };
    }
  }
  throw "Only 'cause -Werror=return-type doesn't recognize, that this function always returns through switch";
}

std::function<tl::expected<std::vector<uint8_t>, std::string>(std::vector<uint8_t> &&)>
getDecodeFnc(bool enableStatic, bool enableModel, bool adaptive) {
  if (enableStatic) {
    if (enableModel) {
      return [](auto &&data) {
        return pf::kko::decodeStatic<uint8_t>(std::move(data), pf::kko::NeighborDifferenceModel<uint8_t>{});
      };
    } else {
      return [](auto &&data) {
        return pf::kko::decodeStatic<uint8_t>(std::move(data), pf::kko::IdentityModel<uint8_t>{});
      };
    }
  }
  if (!adaptive) {
    if (enableModel) {
      return [](auto &&data) {
        return pf::kko::decodeAdaptive<uint8_t>(std::move(data), pf::kko::NeighborDifferenceModel<uint8_t>{});
      };
    } else {
      return [](auto &&data) {
        return pf::kko::decodeAdaptive<uint8_t>(std::move(data), pf::kko::IdentityModel<uint8_t>{});
      };
    }
  } else {
    if (enableModel) {
      return [](auto &&data) {
        return pf::kko::decodeImageAdaptiveBlocks<uint8_t>(std::move(data),
                                                           pf::kko::NeighborDifferenceModel<uint8_t>{});
      };
    } else {
      return [](auto &&data) {
        return pf::kko::decodeImageAdaptiveBlocks<uint8_t>(std::move(data), pf::kko::IdentityModel<uint8_t>{});
      };
    }
  }
  throw "Only 'cause -Werror=return-type doesn't recognize, that this function always returns through switch";
}

std::optional<argparse::ArgumentParser> parseArgs(std::span<char *> args) {
  auto parser = argparse::ArgumentParser("huff_codec_bench");
  parser.add_argument("-i")
      .help("Path to folder with input files")
      .required()
      .action(ValidPathCheckAction{PathType::Directory, true});
  parser.add_argument("--validate").help("Test encoding validity").default_value(false).implicit_value(true);

  try {
    parser.parse_args(args.size(), args.data());
  } catch (const std::runtime_error &err) {
    fmt::print("{}\n", err.what());
    fmt::print("{}", parser);
    return std::nullopt;
  }
  return parser;
}

void benchFile(std::string fileName, const std::vector<uint8_t> &data) {
  using namespace ankerl::nanobench;
  {// encodes
    auto bench = Bench();
    bench.title(fmt::format("Encode bench for file: {}", fileName)).relative(true).warmup(5).performanceCounters(true);
    bench.run("Encode huffman static no model", [data] {
      auto d = data;
      doNotOptimizeAway(getEncodeFnc(true, false, false)(std::move(d)));
    });
    bench.run("Encode huffman static model", [data] {
      auto d = data;
      doNotOptimizeAway(getEncodeFnc(true, true, false)(std::move(d)));
    });
    bench.run("Encode huffman adaptive no model", [data] {
      auto d = data;
      doNotOptimizeAway(getEncodeFnc(false, false, false)(std::move(d)));
    });
    bench.run("Encode huffman adaptive model", [data] {
      auto d = data;
      doNotOptimizeAway(getEncodeFnc(false, true, false)(std::move(d)));
    });
    bench.run("Encode huffman adaptive adaptive no model", [data] {
      auto d = data;
      doNotOptimizeAway(getEncodeFnc(false, false, true)(std::move(d)));
    });
    bench.run("Encode huffman adaptive adaptive model", [data] {
      auto d = data;
      doNotOptimizeAway(getEncodeFnc(false, true, true)(std::move(d)));
    });
  }
  {// decodes
    auto bench = Bench();
    bench.title(fmt::format("Decode bench for file: {}", fileName)).relative(true).warmup(5).performanceCounters(true);
    auto d1 = data;
    auto data1 = getEncodeFnc(true, false, false)(std::move(d1));
    bench.run("Decode huffman static no model", [data1] {
      auto d = data1;
      doNotOptimizeAway(getDecodeFnc(true, false, false)(std::move(d)));
    });
    auto d2 = data;
    auto data2 = getEncodeFnc(true, true, false)(std::move(d2));
    bench.run("Decode huffman static model", [data2] {
      auto d = data2;
      doNotOptimizeAway(getDecodeFnc(true, true, false)(std::move(d)));
    });
    auto d3 = data;
    auto data3 = getEncodeFnc(false, false, false)(std::move(d3));
    bench.run("Decode huffman adaptive no model", [data3] {
      auto d = data3;
      doNotOptimizeAway(getDecodeFnc(false, false, false)(std::move(d)));
    });
    auto d4 = data;
    auto data4 = getEncodeFnc(false, true, false)(std::move(d4));
    bench.run("Decode huffman adaptive model", [data4] {
      auto d = data4;
      doNotOptimizeAway(getDecodeFnc(false, true, false)(std::move(d)));
    });
    auto d5 = data;
    auto data5 = getEncodeFnc(false, false, true)(std::move(d5));
    bench.run("Decode huffman adaptive adaptive no model", [data5] {
      auto d = data5;
      doNotOptimizeAway(getDecodeFnc(false, false, true)(std::move(d)));
    });
    auto d6 = data;
    auto data6 = getEncodeFnc(false, true, true)(std::move(d6));
    bench.run("Decode huffman adaptive adaptive model", [data6] {
      auto d = data6;
      doNotOptimizeAway(getDecodeFnc(false, true, true)(std::move(d)));
    });
  }
}

void testValidity(std::string fileName, const std::vector<uint8_t> &data) {
  using namespace std::string_literals;
  auto cmp = [](const auto &a, const auto &b) {
    if (a.size() != b.size()) { return "wrong length"s; }
    for (std::size_t i = 0; i < a.size(); ++i) {
      if (a[i] != b[i]) { return fmt::format("different data, matched {}/{}", i, a.size()); }
    }
    return "matching"s;
  };
  fmt::print("Checking file: {}\n", fileName);

  auto d = data;
  auto encoded1 = getEncodeFnc(true, false, false)(std::move(d));
  const auto decoded1 = *getDecodeFnc(true, false, false)(std::move(encoded1));
  std::cout << "huffman static no model: " << cmp(decoded1, data) << " original size: " << data.size()
            << "[B] new size: " << encoded1.size() << "[B] BPC: " << countBitsPerCharacter(data.size(), encoded1.size())
            << std::endl;
  d = data;
  auto encoded2 = getEncodeFnc(true, true, false)(std::move(d));
  const auto decoded2 = *getDecodeFnc(true, true, false)(std::move(encoded2));
  std::cout << "huffman static model: " << cmp(decoded2, data) << " original size: " << data.size()
            << "[B] new size: " << encoded2.size() << "[B] BPC: " << countBitsPerCharacter(data.size(), encoded2.size())
            << std::endl;
  d = data;
  auto encoded3 = getEncodeFnc(false, false, false)(std::move(d));
  const auto decoded3 = *getDecodeFnc(false, false, false)(std::move(encoded3));
  std::cout << "huffman adaptive no model: " << cmp(decoded3, data) << " original size: " << data.size()
            << "[B] new size: " << encoded3.size() << "[B] BPC: " << countBitsPerCharacter(data.size(), encoded3.size())
            << std::endl;
  d = data;
  auto encoded4 = getEncodeFnc(false, true, false)(std::move(d));
  const auto decoded4 = *getDecodeFnc(false, true, false)(std::move(encoded4));
  std::cout << "huffman adaptive model: " << cmp(decoded4, data) << " original size: " << data.size()
            << "[B] new size: " << encoded4.size() << "[B] BPC: " << countBitsPerCharacter(data.size(), encoded4.size())
            << std::endl;
  d = data;
  auto encoded5 = getEncodeFnc(false, false, true)(std::move(d));
  const auto decoded5 = *getDecodeFnc(false, false, true)(std::move(encoded5));
  std::cout << "huffman adaptive adaptive no model: " << cmp(decoded5, data) << " original size: " << data.size()
            << "[B] new size: " << encoded5.size() << "[B] BPC: " << countBitsPerCharacter(data.size(), encoded5.size())
            << std::endl;
  d = data;
  auto encoded6 = getEncodeFnc(false, true, true)(std::move(d));
  const auto decoded6 = *getDecodeFnc(false, true, true)(std::move(encoded6));
  std::cout << "huffman adaptive adaptive model: " << cmp(decoded6, data) << " original size: " << data.size()
            << "[B] new size: " << encoded6.size() << "[B] BPC: " << countBitsPerCharacter(data.size(), encoded6.size())
            << std::endl;
  std::cout << "_______________________________________________________________" << std::endl;
}

int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::off);
  auto args = parseArgs(std::span(argv, argc));
  if (!args.has_value()) {
    spdlog::trace("Help or error during arg parsing");
    return 0;
  }

  auto dataDir = args->get<std::filesystem::path>("-i");
  auto inputData = std::vector<std::pair<std::string, std::vector<uint8_t>>>();
  for (const auto &entry : std::filesystem::directory_iterator(dataDir)) {
    if (entry.is_regular_file()) {
      inputData.emplace_back(entry.path().filename(), RawGrayscaleImageDataReader(entry, IMAGE_WIDTH).readAllRaw());
    }
  }
  std::ranges::sort(inputData, [](const auto &lhs, const auto &rhs) { return lhs.first < rhs.first; });

  const auto validate = args->get<bool>("--validate");
  for (const auto &[name, data] : inputData) {
    if (validate) {
      testValidity(name, data);
    } else {
      benchFile(name, data);
    }
  }

  return 0;
}

#endif//HUFF_CODEC__BENCH_H

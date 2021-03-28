//
// Created by petr on 3/28/21.
//

#ifndef HUFF_CODEC__BENCH_H
#define HUFF_CODEC__BENCH_H

#include "RawGrayscaleImageDataReader.h"
#include "args/ValidPathCheckAction.h"
#include "static_decoding.h"
#include "static_encoding.h"
#include <argparse.hpp>
#define ANKERL_NANOBENCH_IMPLEMENT
#include <fmt/ostream.h>
#include <nanobench.h>
#include <span>

enum class AppMode { Compress, Decompress };

struct AppSettings {
  AppMode mode;
  bool enableModel;
  bool adaptiveScanning;
  std::size_t imageWidth;
  std::filesystem::path inputPath;
  std::filesystem::path outputPath;
};

std::optional<argparse::ArgumentParser> parseArgs(std::span<char *> args) {
  auto parser = argparse::ArgumentParser("huff_codec");
  parser.add_argument("-c").help("Compress input file").default_value(false).implicit_value(true);
  parser.add_argument("-d").help("Decompress input file").default_value(false).implicit_value(true);
  parser.add_argument("-m").help("Activate model for preprocessing").default_value(false).implicit_value(true);
  parser.add_argument("-a").help("Adaptive scanning").default_value(false).implicit_value(true);
  parser.add_argument("-i").help("Path to input file").required().action(ValidPathCheckAction{PathType::File, true});
  parser.add_argument("-w").help("Input image width").required().action([](const std::string &value) {
    const auto result = std::stoi(value);
    if (result < 1) { throw std::runtime_error(fmt::format("Invalid value for image width: '{}'", result)); }
    return static_cast<std::size_t>(result);
  });

  try {
    parser.parse_args(args.size(), args.data());
  } catch (const std::runtime_error &err) {
    fmt::print("{}\n", err.what());
    fmt::print("{}", parser);
    return std::nullopt;
  }
  return parser;
}

int main(int argc, char **argv) {
  auto args = parseArgs(std::span(argv, argc));
  if (!args.has_value()) {
    spdlog::trace("Help or error during arg parsing");
    return 0;
  }

  const auto settings = AppSettings{.mode = args->get<bool>("-c") ? AppMode::Compress : AppMode::Decompress,
                                    .enableModel = args->get<bool>("-m"),
                                    .adaptiveScanning = args->get<bool>("-a"),
                                    .imageWidth = args->get<std::size_t>("-w"),
                                    .inputPath = args->get<std::filesystem::path>("-i"),
                                    .outputPath = {}};

  auto data = pf::kko::RawGrayscaleImageDataReader{settings.inputPath, settings.imageWidth}.readAllRaw();

  auto bench = ankerl::nanobench::Bench();
  bench.title("huff_code benchmark").warmup(100).relative(false).performanceCounters(true).epochIterations(10000);
  switch (settings.mode) {
    case AppMode::Compress: {
      auto encodedData = std::vector<uint8_t>{};
      if (settings.enableModel) {
        bench.run([&] {
          auto d = data;
          ankerl::nanobench::doNotOptimizeAway(
              pf::kko::encodeStatic<uint8_t>(std::move(d), pf::kko::NeighborDifferenceModel<uint8_t>{}));
        });
      } else {
        bench.run([&] {
          auto d = data;
          ankerl::nanobench::doNotOptimizeAway(pf::kko::encodeStatic<uint8_t>(std::move(d)));
        });
      }

    } break;
    case AppMode::Decompress: {
      auto decodedData = tl::expected<std::vector<uint8_t>, std::string>{};
      if (settings.enableModel) {
        bench.run([&] {
          auto d = data;
          ankerl::nanobench::doNotOptimizeAway(
              pf::kko::decodeStatic<uint8_t>(std::move(data), pf::kko::NeighborDifferenceModel<uint8_t>{}));
        });
      } else {
        bench.run([&] {
          auto d = data;
          ankerl::nanobench::doNotOptimizeAway(pf::kko::decodeStatic<uint8_t>(std::move(data)));
        });
      }
    } break;
  }
}

#endif//HUFF_CODEC__BENCH_H

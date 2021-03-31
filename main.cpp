#define FMT_HEADER_ONLY
#include "RawGrayscaleImageDataReader.h"
#include "Tree.h"
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

// ovladani logovani - zapisuje pouze do stdout
#define ENABLE_LOG 1

enum class AppMode { Compress, Decompress };
enum class CompressionType { Static, Adaptive };

struct AppSettings {
  AppMode mode;
  bool enableModel;
  CompressionType compressionType;
  std::size_t imageWidth;
  std::filesystem::path inputPath;
  std::filesystem::path outputPath;
};

/**
 * Zpracovani vstupnich argumentu programu
 * @param args vstupni argumenty programu
 * @return std::nullopt, pokud doslo k chybe zpracovani argumentu, jinak objekt obsahujici informace o zpracovanych argumentech
 */
std::optional<argparse::ArgumentParser> parseArgs(std::span<char *> args) {
  auto parser = argparse::ArgumentParser("huff_codec");
  parser.add_argument("-c").help("Compress input file").default_value(false).implicit_value(true);
  parser.add_argument("-d").help("Decompress input file").default_value(false).implicit_value(true);
  parser.add_argument("-m").help("Activate model for preprocessing").default_value(false).implicit_value(true);
  parser.add_argument("-a")
      .help("Adaptive scanning")
      .default_value(CompressionType::Static)
      .implicit_value(CompressionType::Adaptive);
  parser.add_argument("-i").help("Path to input file").required().action(ValidPathCheckAction{PathType::File, true});
  parser.add_argument("-o").help("Path to output file").required().action(ValidPathCheckAction{PathType::File, false});
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

std::function<std::vector<uint8_t>(std::vector<uint8_t> &&)> getEncodeFnc(const AppSettings &settings) {
  switch (settings.compressionType) {
    case CompressionType::Static: {
      if (settings.enableModel) {
        return [](auto &&data) {
          return pf::kko::encodeStatic<uint8_t>(std::move(data), pf::kko::NeighborDifferenceModel<uint8_t>{});
        };
      } else {
        return [](auto &&data) { return pf::kko::encodeStatic<uint8_t>(std::move(data)); };
      }
    }
    case CompressionType::Adaptive: {
      if (settings.enableModel) {
        return [](auto &&data) {
          return pf::kko::encodeAdaptive<uint8_t>(std::move(data), pf::kko::NeighborDifferenceModel<uint8_t>{});
        };
      } else {
        return [](auto &&data) { return pf::kko::encodeAdaptive<uint8_t>(std::move(data)); };
      }
    }
  }
  throw "Only 'cause -Werror=return-type doesn't recognize, that this function always returns through switch";
}

std::function<tl::expected<std::vector<uint8_t>, std::string>(std::vector<uint8_t> &&)>
getDecodeFnc(const AppSettings &settings) {
  switch (settings.compressionType) {
    case CompressionType::Static: {
      if (settings.enableModel) {
        return [](auto &&data) {
          return pf::kko::decodeStatic<uint8_t>(std::move(data), pf::kko::NeighborDifferenceModel<uint8_t>{});
        };
      } else {
        return [](auto &&data) { return pf::kko::decodeStatic<uint8_t>(std::move(data)); };
      }
    }
    case CompressionType::Adaptive: {
      if (settings.enableModel) {
        return [](auto &&data) {
          return pf::kko::decodeAdaptive<uint8_t>(std::move(data), pf::kko::NeighborDifferenceModel<uint8_t>{});
        };
      } else {
        return [](auto &&data) { return pf::kko::decodeAdaptive<uint8_t>(std::move(data)); };
      }
    }
  }
  throw "Only 'cause -Werror=return-type doesn't recognize, that this function always returns through switch";
}

int main(int argc, char **argv) {
  spdlog::set_pattern("[%H:%M:%S.%f] [%l] [thread %t] %v");
#if ENABLE_LOG == 0
  spdlog::set_level(spdlog::level::off);
#else
  spdlog::set_level(spdlog::level::trace);
#endif
  spdlog::trace("Parsing arguments");
  auto args = parseArgs(std::span(argv, argc));
  if (!args.has_value()) {
    spdlog::trace("Help or error during arg parsing");
    return 0;
  }

  const auto settings = AppSettings{.mode = args->get<bool>("-c") ? AppMode::Compress : AppMode::Decompress,
                                    .enableModel = args->get<bool>("-m"),
                                    .compressionType = args->get<CompressionType>("-a"),
                                    .imageWidth = args->get<std::size_t>("-w"),
                                    .inputPath = args->get<std::filesystem::path>("-i"),
                                    .outputPath = args->get<std::filesystem::path>("-o")};

  spdlog::info("Input file: {}, image width: {}, mode: {}, using model: {}", settings.inputPath.string(),
               settings.imageWidth, magic_enum::enum_name(settings.mode), settings.enableModel);

  auto data = pf::kko::RawGrayscaleImageDataReader{settings.inputPath, settings.imageWidth}.readAllRaw();
  spdlog::trace("Read data, total length: {}[B]", data.size());

  auto outputStream = std::ofstream(settings.outputPath, std::ios::binary);

  switch (settings.mode) {
    case AppMode::Compress: {
      const auto encodedData = getEncodeFnc(settings)(std::move(data));
      outputStream.write(reinterpret_cast<const char *>(encodedData.data()), encodedData.size());
    } break;
    case AppMode::Decompress: {
      const auto decodedData = getDecodeFnc(settings)(std::move(data));
      if (decodedData.has_value()) {
        outputStream.write(reinterpret_cast<const char *>(decodedData->data()), decodedData->size());
      } else {
        spdlog::error("Error while decoding: {}", decodedData.error());
        fmt::print(stderr, "Error while decoding: {}", decodedData.error());
      }
    } break;
  }

  return 0;
}

#include "RawGrayscaleImageDataReader.h"
#include "argparse.hpp"
#include "args/ValidPathCheckAction.h"
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <optional>
#include <span>

enum class AppMode { Compress, Decompress };

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
  parser.add_argument("-a").help("Activate adaptive scanning").default_value(false).implicit_value(true);
  parser.add_argument("-i").help("Path to input file").required().action(ValidPathCheckAction{PathType::File, true});
  parser.add_argument("-o").help("Path to output file").required().action(ValidPathCheckAction{PathType::File, false});
  parser.add_argument("-w").help("Input image width").required().action([](const std::string &value) {
    const auto result = std::stoi(value);
    if (result < 1) { throw std::runtime_error(fmt::format("Invalid value for image width: '{}'", result)); }
    return result;
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
  if (!args.has_value()) { return 0; }

  const auto inputFilePath = args->get<std::filesystem::path>("-i");
  const auto imageWidth = args->get<int>("-w");
  auto data = RawGrayscaleImageDataReader{inputFilePath, imageWidth};

  return 0;
}

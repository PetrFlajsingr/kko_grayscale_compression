#include "argparse.hpp"
#include "args/ValidPathCheckAction.h"
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <optional>
#include <span>

std::optional<argparse::ArgumentParser> parseArgs(std::span<char *> args) {
  auto parser = argparse::ArgumentParser("KKO 2021 xflajs00");
  parser.add_argument("-c").help("Compress input file").default_value(false).implicit_value(true);
  parser.add_argument("-d").help("Decompress input file").default_value(false).implicit_value(true);
  parser.add_argument("-m").help("Activate model for preprocessing").default_value(false).implicit_value(true);
  parser.add_argument("-a").help("Activate adaptive scanning").default_value(false).implicit_value(true);
  parser.add_argument("-i").help("Path to input file").required().action(ValidPathCheckAction{PathType::File});
  parser.add_argument("-o").help("Path to output file").required().action(ValidPathCheckAction{PathType::File});
  parser.add_argument("-w").help("Input image width").required().action([](const std::string &value) {
    return std::stoi(value);
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
  const auto args = parseArgs(std::span(argv, argc));
  if (!args.has_value()) { return 0; }

  return 0;
}

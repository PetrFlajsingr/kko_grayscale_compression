//
// Created by petr on 9/23/20.
//

#ifndef VOXEL_RENDER_VALIDPATHCHECKACTION_H
#define VOXEL_RENDER_VALIDPATHCHECKACTION_H
#include "fmt/format.h"
#include <filesystem>

enum class PathType { File, Directory };

struct ValidPathCheckAction {
  explicit inline ValidPathCheckAction(PathType pathType, bool mustExist) : type(pathType), exists(mustExist) {}
  [[nodiscard]] inline std::filesystem::path operator()(std::string_view pathStr) const {
    auto path = std::filesystem::path(pathStr);
    if (exists) {
      if (type == PathType::Directory && !std::filesystem::is_directory(path)) {
        throw std::runtime_error{fmt::format("Provided path: '{}' is not a directory.", pathStr)};
      }
      if (type == PathType::File && !std::filesystem::is_regular_file(path)) {
        throw std::runtime_error{fmt::format("Provided path: '{}' is not a file.", pathStr)};
      }
    }
    return path;
  }
  const PathType type;
  const bool exists;
};

#endif//VOXEL_RENDER_VALIDPATHCHECKACTION_H

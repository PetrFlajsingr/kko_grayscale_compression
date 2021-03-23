//
// Created by petr on 9/23/20.
//

#ifndef VOXEL_RENDER_VALIDPATHCHECKACTION_H
#define VOXEL_RENDER_VALIDPATHCHECKACTION_H
#include "fmt/format.h"
#include <filesystem>

enum class PathType { File, Directory };

struct ValidPathCheckAction {
  explicit ValidPathCheckAction(PathType pathType, bool mustExist);
  std::filesystem::path operator()(std::string_view pathStr) const;
  const PathType type;
  const bool exists;
};

#endif//VOXEL_RENDER_VALIDPATHCHECKACTION_H

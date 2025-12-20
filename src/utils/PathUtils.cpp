#include "PathUtils.hpp"

std::string joinPaths(const std::string &base, const std::string &relative) {
  if (relative.empty())
    return base;
  std::string rel = relative;
  if (!rel.empty() && rel[0] == '/')
    rel.erase(0, 1);
  if (base.empty())
    return rel;
  if (base[base.size() - 1] == '/')
    return base + rel;
  return base + "/" + rel;
}

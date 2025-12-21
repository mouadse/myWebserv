#include "Delete.hpp"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <limits.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace {
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

std::string dirnameOf(const std::string &path) {
  std::string::size_type slash = path.rfind('/');
  if (slash == std::string::npos)
    return ".";
  if (slash == 0)
    return "/";
  return path.substr(0, slash);
}

bool isPathUnderRoot(const std::string &root, const std::string &path) {
  if (root.empty())
    return false;
  std::string prefix = root;
  if (prefix[prefix.size() - 1] != '/')
    prefix += "/";
  return path.compare(0, prefix.size(), prefix) == 0;
}

void setHtmlError(HttpResponse &response, int code, const std::string &message,
                  const std::string &detail) {
  response.setStatus(code, message);
  std::stringstream body;
  body << "<html><head><title>" << code << " " << message << "</title></head>"
       << "<body><h1>" << code << " " << message << "</h1><p>" << detail
       << "</p></body></html>";
  response.setBody(body.str());
  response.setHeader("Content-Type", "text/html");
}
}

void Delete::handle(const HTTPRequest &request, HttpResponse &response,
                    const LocationBlock &location) {
  const std::string uri = request.getTarget();
  if (uri.empty() || uri[0] != '/') {
    setHtmlError(response, 400, "Bad Request", "Invalid request target.");
    return;
  }
  if (uri == "/") {
    setHtmlError(response, 403, "Forbidden", "Cannot delete server root.");
    return;
  }

  const std::string root = location.getRoot();
  const std::string filePath = joinPaths(root, uri);
  const std::string dirPath = dirnameOf(filePath);

  char rootReal[PATH_MAX];
  if (realpath(root.c_str(), rootReal) == NULL) {
    setHtmlError(response, 500, "Internal Server Error",
                 "Failed to resolve server root.");
    return;
  }

  char dirReal[PATH_MAX];
  if (realpath(dirPath.c_str(), dirReal) == NULL) {
    setHtmlError(response, 404, "Not Found", "Target not found.");
    return;
  }
  if (!isPathUnderRoot(rootReal, dirReal)) {
    setHtmlError(response, 403, "Forbidden", "Path traversal detected.");
    return;
  }

  struct stat fileStat;
  if (stat(filePath.c_str(), &fileStat) != 0) {
    if (errno == ENOENT)
      setHtmlError(response, 404, "Not Found", "Target not found.");
    else
      setHtmlError(response, 403, "Forbidden", "Cannot access target.");
    return;
  }

  if (S_ISDIR(fileStat.st_mode)) {
    setHtmlError(response, 403, "Forbidden", "Cannot delete a directory.");
    return;
  }

  if (access(dirPath.c_str(), W_OK | X_OK) != 0) {
    setHtmlError(response, 403, "Forbidden", "Permission denied.");
    return;
  }

  if (std::remove(filePath.c_str()) != 0) {
    if (errno == ENOENT)
      setHtmlError(response, 404, "Not Found", "Target not found.");
    else if (errno == EACCES || errno == EPERM)
      setHtmlError(response, 403, "Forbidden", "Permission denied.");
    else
      setHtmlError(response, 500, "Internal Server Error",
                   "Failed to delete target.");
    return;
  }

  response.setStatus(204, "No Content");
  response.setBody("");
}

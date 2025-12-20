#include "Post.hpp"
#include "../../utils/PathUtils.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {
void setError(HttpResponse &response, int code, const std::string &message) {
  response.setStatus(code, message);
  std::stringstream body;
  body << "<html><body><h1>" << code << " " << message << "</h1></body></html>";
  response.setBody(body.str());
  response.setHeader("Content-Type", "text/html");
}

std::string sanitizeFilename(const std::string &filename) {
  std::string base = filename;
  size_t lastSlash = base.find_last_of("/\\");
  if (lastSlash != std::string::npos) {
    base = base.substr(lastSlash + 1);
  }

  std::string sanitized;
  std::string allowed =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_";
  for (size_t i = 0; i < base.size(); ++i) {
    if (allowed.find(base[i]) != std::string::npos) {
      sanitized += base[i];
    }
  }

  if (sanitized.empty() || sanitized == "." || sanitized == "..") {
    return "";
  }
  if (sanitized.length() > 255) {
    sanitized = sanitized.substr(0, 255);
  }
  return sanitized;
}
} // namespace

void Post::handle(const HTTPRequest &request, HttpResponse &response,
                  const LocationBlock &location) {
  std::string contentType = request.getHeader("Content-Type");

  // Support raw binary POST if Content-Type is NOT multipart
  if (contentType.find("multipart/form-data") == std::string::npos) {
    // Use the same path resolution logic as Get::handle
    std::string target = request.getTarget();
    std::string path;
    std::string locationPath = location.getPath();

    if (!location.getAlias().empty() && target.rfind(locationPath, 0) == 0) {
      // If alias is defined and target starts with locationPath, use alias
      std::string relativeTarget = target.substr(locationPath.length());
      path = joinPaths(location.getAlias(), relativeTarget);
    } else {
      // Otherwise use root
      path = joinPaths(location.getRoot(), target);
    }

    // Check if path is a directory or ends with '/' - reject with 400
    struct stat pathStat;
    bool pathExists = (stat(path.c_str(), &pathStat) == 0);

    if (pathExists && S_ISDIR(pathStat.st_mode)) {
      setError(response, 400,
               "Bad Request: Cannot POST binary data to a directory");
      return;
    }

    if (!target.empty() && target[target.length() - 1] == '/') {
      setError(
          response, 400,
          "Bad Request: Cannot POST binary data to a path ending with '/'");
      return;
    }

    // Write raw request body to target path
    const std::vector<char> &body = request.getBody();
    std::ofstream outFile(path.c_str(), std::ios::binary);
    if (!outFile) {
      std::cerr << "Error: Could not open file for writing at " << path
                << std::endl;
      setError(response, 500,
               "Internal Server Error: Could not open file for writing");
      return;
    }

    if (!body.empty()) {
      outFile.write(&body[0], body.size());
    }
    outFile.close();

    response.setStatus(201, "Created");
    std::stringstream resBody;
    resBody << "<html><body><h1>File Uploaded Successfully</h1><p>Saved to "
            << path << "</p></body></html>";
    response.setBody(resBody.str());
    response.setHeader("Content-Type", "text/html");
    response.setHeader("Location", target);
    return;
  }

  size_t boundaryPos = contentType.find("boundary=");
  if (boundaryPos == std::string::npos) {
    setError(response, 400, "Bad Request: Boundary missing");
    return;
  }

  std::string boundaryParam = contentType.substr(boundaryPos + 9);
  size_t semiPos = boundaryParam.find(';');
  if (semiPos != std::string::npos) {
    boundaryParam = boundaryParam.substr(0, semiPos);
  }

  // Trim whitespace
  size_t first = boundaryParam.find_first_not_of(" \t");
  if (first == std::string::npos) {
    setError(response, 400, "Bad Request: Empty boundary");
    return;
  }
  size_t last = boundaryParam.find_last_not_of(" \t");
  boundaryParam = boundaryParam.substr(first, (last - first + 1));

  // Remove surrounding quotes if present
  if (boundaryParam.size() >= 2 && boundaryParam[0] == '"' &&
      boundaryParam[boundaryParam.size() - 1] == '"') {
    boundaryParam = boundaryParam.substr(1, boundaryParam.size() - 2);
  }

  std::string boundary = "--" + boundaryParam;
  const std::vector<char> &body = request.getBody();

  // Find first boundary
  std::vector<char>::const_iterator it =
      std::search(body.begin(), body.end(), boundary.begin(), boundary.end());
  if (it == body.end()) {
    setError(response, 400, "Bad Request: Start boundary not found");
    return;
  }

  // Move past the first boundary + CRLF (assuming \r\n)
  // Note: The boundary might be followed by \r\n
  if (std::distance(it, body.end()) < static_cast<long>(boundary.size())) {
    setError(response, 400, "Bad Request: Malformed body");
    return;
  }
  it += boundary.size();
  if (it != body.end() && *it == '\r')
    it++;
  if (it != body.end() && *it == '\n')
    it++;

  // Find headers end (\r\n\r\n)
  const char *crlf2 = "\r\n\r\n";
  std::vector<char>::const_iterator headerEnd =
      std::search(it, body.end(), crlf2, crlf2 + 4);
  if (headerEnd == body.end()) {
    setError(response, 400, "Bad Request: Header end not found");
    return;
  }

  // Parse headers to find filename
  std::string headers(it, headerEnd);
  size_t filenamePos = headers.find("filename=\"");
  if (filenamePos == std::string::npos) {
    setError(response, 400, "Bad Request: Filename not found");
    return;
  }

  filenamePos += 10;
  size_t filenameEnd = headers.find("\"", filenamePos);
  if (filenameEnd == std::string::npos) {
    setError(response, 400, "Bad Request: Filename parsing error");
    return;
  }
  std::string rawFilename =
      headers.substr(filenamePos, filenameEnd - filenamePos);
  std::string filename = sanitizeFilename(rawFilename);
  if (filename.empty()) {
    setError(response, 400, "Bad Request: Invalid filename");
    return;
  }

  // Content starts after \r\n\r\n
  std::vector<char>::const_iterator contentStart = headerEnd + 4;

  // Find next boundary
  std::vector<char>::const_iterator contentEnd =
      std::search(contentStart, body.end(), boundary.begin(), boundary.end());

  // Check if there is a CRLF before the boundary
  if (contentEnd != body.begin()) {
    std::vector<char>::const_iterator temp = contentEnd;
    temp--;
    if (*temp == '\n') {
      if (temp != body.begin()) {
        std::vector<char>::const_iterator prev = temp;
        prev--;
        if (*prev == '\r') {
          contentEnd = prev;
        } else {
          contentEnd = temp;
        }
      } else {
        contentEnd = temp;
      }
    }
  }

  std::string uploadPath;
  std::string locationPath = location.getPath();

  if (!location.getAlias().empty() &&
      request.getTarget().rfind(locationPath, 0) ==
          0) { // Check if target starts with locationPath
    // If an alias is defined and the target matches the location path, use the
    // alias. The target needs to have the location block's path stripped and
    // then the alias prepended.
    std::string relativeTarget =
        request.getTarget().substr(locationPath.length());
    uploadPath = joinPaths(location.getAlias(), relativeTarget);
  } else {
    // Otherwise, use the root as before.
    uploadPath = joinPaths(location.getRoot(), request.getTarget());
  }

  std::string path = joinPaths(uploadPath, filename);

  // Ensure the directory exists or at least matches the location root/target
  // Here we assume the target directory exists as per assignment simplicity or
  // manual setup.

  std::ofstream outFile(path.c_str(), std::ios::binary);
  if (!outFile) {
    std::cerr << "Error: Could not open file for writing at " << path
              << std::endl;
    setError(response, 500,
             "Internal Server Error: Could not open file for writing");
    return;
  }

  if (contentStart != body.end() && contentStart < contentEnd) {
    outFile.write(&(*contentStart), std::distance(contentStart, contentEnd));
  }
  outFile.close();

  response.setStatus(201, "Created");
  std::stringstream resBody;
  resBody << "<html><body><h1>File Uploaded Successfully</h1><p>Saved as "
          << filename << "</p></body></html>";
  response.setBody(resBody.str());
  response.setHeader("Content-Type", "text/html");
  response.setHeader("Location", joinPaths(request.getTarget(), filename));
}

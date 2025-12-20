#include "Post.hpp"
#include "../../utils/PathUtils.hpp"
#include <algorithm>
#include <cstring>
#include <ctime>
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

std::string generateTimestampFilename(const std::string &contentType) {
  std::time_t now = std::time(NULL);
  std::stringstream ss;
  ss << "upload_" << now;

  // Try to extract extension from Content-Type (e.g., "image/png" -> ".png")
  std::string ext = ".bin";
  size_t slashPos = contentType.find('/');
  if (slashPos != std::string::npos) {
    std::string subtype = contentType.substr(slashPos + 1);
    // Remove any parameters after semicolon
    size_t semiPos = subtype.find(';');
    if (semiPos != std::string::npos) {
      subtype = subtype.substr(0, semiPos);
    }
    // Trim whitespace
    size_t start = subtype.find_first_not_of(" \t");
    size_t end = subtype.find_last_not_of(" \t");
    if (start != std::string::npos && end != std::string::npos) {
      subtype = subtype.substr(start, end - start + 1);
    }
    // Map common MIME types to extensions
    if (subtype == "jpeg") {
      ext = ".jpg";
    } else if (subtype == "plain") {
      ext = ".txt";
    } else if (subtype == "octet-stream") {
      ext = ".bin";
    } else if (!subtype.empty() && subtype.length() <= 10) {
      ext = "." + subtype;
    }
  }

  ss << ext;
  return ss.str();
}
} // namespace

void Post::handle(const HTTPRequest &request, HttpResponse &response,
                  const LocationBlock &location) {
  std::string contentType = request.getHeader("Content-Type");

  if (contentType.find("multipart/form-data") == std::string::npos) {
    std::string target = request.getTarget();
    std::string path;
    std::string locationPath = location.getPath();

    if (!location.getAlias().empty() && target.rfind(locationPath, 0) == 0) {
      std::string relativeTarget = target.substr(locationPath.length());
      path = joinPaths(location.getAlias(), relativeTarget);
    } else {
      path = joinPaths(location.getRoot(), target);
    }

    struct stat pathStat;
    bool pathExists = (stat(path.c_str(), &pathStat) == 0);

    bool isDirectory = (pathExists && S_ISDIR(pathStat.st_mode));
    bool endsWithSlash =
        (!target.empty() && target[target.length() - 1] == '/');

    if (isDirectory || endsWithSlash) {
      if (endsWithSlash && !pathExists) {
        std::cerr << "Error: Directory does not exist at " << path << std::endl;
        setError(response, 404, "Not Found: Target directory does not exist");
        return;
      }
      if (endsWithSlash && !S_ISDIR(pathStat.st_mode)) {
        std::cerr << "Error: Path is not a directory at " << path << std::endl;
        setError(response, 400,
                 "Bad Request: Target path is not a directory");
        return;
      }
      std::string generatedFilename = generateTimestampFilename(contentType);
      path = joinPaths(path, generatedFilename);
      target = joinPaths(target, generatedFilename);
    }

    const std::vector<char> &body = request.getBody();
    std::ofstream outFile(path.c_str(), std::ios::binary);
    if (!outFile) {
      std::cerr << "Error: Could not open file for writing at " << path
                << std::endl;
      setError(response, 500,
               "Internal Server Error: Could not create file in target path");
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

  size_t first = boundaryParam.find_first_not_of(" \t");
  if (first == std::string::npos) {
    setError(response, 400, "Bad Request: Empty boundary");
    return;
  }
  size_t last = boundaryParam.find_last_not_of(" \t");
  boundaryParam = boundaryParam.substr(first, (last - first + 1));

  if (boundaryParam.size() >= 2 && boundaryParam[0] == '"' &&
      boundaryParam[boundaryParam.size() - 1] == '"') {
    boundaryParam = boundaryParam.substr(1, boundaryParam.size() - 2);
  }

  std::string boundary = "--" + boundaryParam;
  const std::vector<char> &body = request.getBody();

  std::vector<char>::const_iterator it =
      std::search(body.begin(), body.end(), boundary.begin(), boundary.end());
  if (it == body.end()) {
    setError(response, 400, "Bad Request: Start boundary not found");
    return;
  }

  if (std::distance(it, body.end()) < static_cast<long>(boundary.size())) {
    setError(response, 400, "Bad Request: Malformed body");
    return;
  }
  it += boundary.size();
  if (it != body.end() && *it == '\r')
    it++;
  if (it != body.end() && *it == '\n')
    it++;

  const char *crlf2 = "\r\n\r\n";
  std::vector<char>::const_iterator headerEnd =
      std::search(it, body.end(), crlf2, crlf2 + 4);
  if (headerEnd == body.end()) {
    setError(response, 400, "Bad Request: Header end not found");
    return;
  }

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

  std::vector<char>::const_iterator contentStart = headerEnd + 4;

  std::vector<char>::const_iterator contentEnd =
      std::search(contentStart, body.end(), boundary.begin(), boundary.end());

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
      request.getTarget().rfind(locationPath, 0) == 0) {
    std::string relativeTarget =
        request.getTarget().substr(locationPath.length());
    uploadPath = joinPaths(location.getAlias(), relativeTarget);
  } else {
    uploadPath = joinPaths(location.getRoot(), request.getTarget());
  }

  std::string path = joinPaths(uploadPath, filename);

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

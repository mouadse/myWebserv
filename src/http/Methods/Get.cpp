#include "Get.hpp"
#include "../../utils/Helpers.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <map>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

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

void setHtmlError(HttpResponse &response, int code, const std::string &message,
                  const std::string &detail = "") {
  response.setStatus(code, message);
  std::stringstream body;
  body << "<html><head><title>" << code << " " << message << "</title></head>"
       << "<body><h1>" << code << " " << message << "</h1>";
  if (!detail.empty()) {
    body << "<p>" << detail << "</p>";
  }
  body << "</body></html>";
  response.setBody(body.str());
  response.setHeader("Content-Type", "text/html");
}

std::string getMimeType(const std::string &path) {
  size_t dotPos = path.rfind('.');
  if (dotPos == std::string::npos)
    return "application/octet-stream";
  std::string ext = path.substr(dotPos + 1);
  // to lower case
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  if (ext == "html" || ext == "htm")
    return "text/html";
  if (ext == "css")
    return "text/css";
  if (ext == "js")
    return "application/javascript";
  if (ext == "jpg" || ext == "jpeg")
    return "image/jpeg";
  if (ext == "png")
    return "image/png";
  if (ext == "gif")
    return "image/gif";
  if (ext == "ico")
    return "image/x-icon";
  if (ext == "txt")
    return "text/plain";
  if (ext == "json")
    return "application/json";
  if (ext == "xml")
    return "application/xml";
  if (ext == "pdf")
    return "application/pdf";
  if (ext == "mp4")
    return "video/mp4";
  if (ext == "webm")
    return "video/webm";
  if (ext == "ogg")
    return "video/ogg";
  if (ext == "avi")
    return "video/x-msvideo";
  if (ext == "mkv")
    return "video/x-matroska";

  return "application/octet-stream";
}

std::string generateDirectoryListing(const std::string &path,
                                     const std::string &requestTarget) {
  DIR *dir;
  struct dirent *ent;
  std::stringstream ss;

  ss << "<html><head><title>Index of " << requestTarget
     << "</title></head><body>";
  ss << "<h1>Index of " << requestTarget << "</h1><hr><pre>";

  if ((dir = opendir(path.c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      std::string name = ent->d_name;
      if (name == ".")
        continue;

      std::string href = name;
      if (ent->d_type == DT_DIR) {
        name += "/";
        href += "/";
      }

      name = Helpers::escapeHtml(name);
      href = Helpers::escapeHtml(href);

      // Simple formatting
      ss << "<a href=\"" << href << "\">" << name << "</a><br>";
    }
    closedir(dir);
  } else {
    return ""; // Error handled by caller
  }

  ss << "</pre><hr></body></html>";
  return ss.str();
}
} // namespace

void Get::handle(const HTTPRequest &request, HttpResponse &response,
                 const LocationBlock &location) {
  std::string target = request.getTarget();

  std::string path;
  std::string locationPath = location.getPath();

  if (!location.getAlias().empty() &&
      target.rfind(locationPath, 0) ==
          0) { // Check if target starts with locationPath
    // If an alias is defined and the target matches the location path, use the
    // alias. The target needs to have the location block's path stripped and
    // then the alias prepended.
    std::string relativeTarget = target.substr(locationPath.length());
    path = joinPaths(location.getAlias(), relativeTarget);
  } else {
    // Otherwise, use the root as before.
    path = joinPaths(location.getRoot(), target);
  }

  struct stat pathStat;
  if (stat(path.c_str(), &pathStat) != 0) {
    setHtmlError(response, 404, "Not Found",
                 "The requested resource was not found on this server.");
    return;
  }

  if (S_ISDIR(pathStat.st_mode)) {
    // It's a directory

    // 1. Check for index file
    std::string indexFile = location.getIndex();
    if (!indexFile.empty()) {
      std::string indexPath = joinPaths(path, indexFile);
      struct stat indexStat;
      if (stat(indexPath.c_str(), &indexStat) == 0 &&
          !S_ISDIR(indexStat.st_mode)) {
        // Serve index file
        path = indexPath;
        // Fall through to file serving logic
        goto serve_file;
      }
    }

    // 2. Check Autoindex
    if (location.getAutoindex()) {
      std::string listing = generateDirectoryListing(path, target);
      if (listing.empty()) {
        setHtmlError(response, 500, "Internal Server Error",
                     "Could not list directory.");
        return;
      }
      response.setStatus(200, "OK");
      response.setBody(listing);
      response.setHeader("Content-Type", "text/html");
      return;
    }

    // 3. Forbidden
    setHtmlError(response, 403, "Forbidden", "Directory listing is disabled.");
    return;
  }

serve_file:
  // It's a file
  std::ifstream file(path.c_str(), std::ios::binary);
  if (!file) {
    if (access(path.c_str(), R_OK) != 0) {
      setHtmlError(response, 403, "Forbidden", "Permission denied.");
    } else {
      setHtmlError(response, 500, "Internal Server Error",
                   "Could not open file.");
    }
    return;
  }

  // Get file size
  file.seekg(0, std::ios::end);
  std::streamsize fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  if (fileSize == -1) {
    setHtmlError(response, 500, "Internal Server Error",
                 "Could not determine file size.");
    return;
  }

  // Range Header Handling
  std::string rangeHeader = request.getHeader("Range");
  if (!rangeHeader.empty() && rangeHeader.compare(0, 6, "bytes=") == 0) {
    std::string rangeSet = rangeHeader.substr(6);
    size_t dashPos = rangeSet.find('-');

    size_t start = 0;
    size_t end = (size_t)fileSize - 1;

    if (dashPos != std::string::npos) {
      std::string startStr = rangeSet.substr(0, dashPos);
      std::string endStr = rangeSet.substr(dashPos + 1);

      if (startStr.empty()) {
        // Suffix: bytes=-500
        if (!endStr.empty()) {
          size_t suffix;
          std::stringstream ss(endStr);
          ss >> suffix;
          if (ss.fail()) {
             setHtmlError(response, 400, "Bad Request", "Invalid Range suffix.");
             return;
          }
          if (suffix < (size_t)fileSize)
            start = (size_t)fileSize - suffix;
          else
            start = 0;
        }
      } else {
        std::stringstream ss(startStr);
        ss >> start;
        if (ss.fail()) {
            setHtmlError(response, 400, "Bad Request", "Invalid Range start.");
            return;
        }
        if (!endStr.empty()) {
          std::stringstream ss2(endStr);
          ss2 >> end;
          if (ss2.fail()) {
              setHtmlError(response, 400, "Bad Request", "Invalid Range end.");
              return;
          }
        }
      }
    }

    // Validate
    if (start >= (size_t)fileSize || start > end) {
      response.setStatus(416, "Range Not Satisfiable");
      std::stringstream cr;
      cr << "bytes */" << fileSize;
      response.setHeader("Content-Range", cr.str());
      return;
    }

    // Cap chunk size (e.g., 1MB)
    const size_t MAX_CHUNK = 1024 * 1024;
    if (end - start + 1 > MAX_CHUNK) {
      end = start + MAX_CHUNK - 1;
    }

    // Final bounds check
    if (end >= (size_t)fileSize)
      end = (size_t)fileSize - 1;

    std::streamsize length = end - start + 1;

    file.seekg(start);
    std::vector<char> buf(length);
    if (file.read(&buf[0], length)) {
      response.setStatus(206, "Partial Content");
      response.setBody(buf);

      std::stringstream cr;
      cr << "bytes " << start << "-" << end << "/" << fileSize;
      response.setHeader("Content-Range", cr.str());

      std::stringstream cl;
      cl << length;
      response.setHeader("Content-Length", cl.str());
      response.setHeader("Content-Type", getMimeType(path));
      return;
    }
  }

  // For files without Range header:
  // Always advertise Accept-Ranges so browser knows it can request ranges
  response.setHeader("Accept-Ranges", "bytes");

  // 80/20 Optimization: For large files (>1MB), send only the first chunk
  // This makes initial load blazingly fast for large videos!
  // The browser will automatically request remaining chunks via Range requests
  const size_t LARGE_FILE_THRESHOLD = 1024 * 1024; // 1MB
  const size_t INITIAL_CHUNK_SIZE = 1024 * 1024;   // 1MB initial chunk

  if ((size_t)fileSize > LARGE_FILE_THRESHOLD) {
    // Large file: send first chunk as 206 Partial Content
    // This makes initial video load blazingly fast!
    file.seekg(0, std::ios::beg);
    size_t chunkSize = std::min((size_t)fileSize, INITIAL_CHUNK_SIZE);
    std::vector<char> buf(chunkSize);

    if (file.read(&buf[0], chunkSize)) {
      response.setStatus(206, "Partial Content");
      response.setBody(buf);
      response.setHeader("Content-Type", getMimeType(path));

      // Content-Range: bytes 0-(chunkSize-1)/totalSize
      std::stringstream cr;
      cr << "bytes 0-" << (chunkSize - 1) << "/" << fileSize;
      response.setHeader("Content-Range", cr.str());

      // Content-Length is the actual chunk size being sent
      std::stringstream cl;
      cl << chunkSize;
      response.setHeader("Content-Length", cl.str());
      return;
    }
  }

  // Small file: load entirely (original behavior)
  file.seekg(0, std::ios::beg);
  std::vector<char> fileBuffer(fileSize);
  if (file.read(&fileBuffer[0], fileSize)) {
    response.setBody(fileBuffer);
    response.setStatus(200, "OK");
    response.setHeader("Content-Type", getMimeType(path));
  } else {
    setHtmlError(response, 500, "Internal Server Error",
                 "Could not read file.");
  }
}

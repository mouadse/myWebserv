#include "Get.hpp"
#include <fstream>
#include <dirent.h>
#include <cstring>

// Simple directory utilities for autoindexing
static std::vector<std::string> getDirList(const std::string &path) {
  std::vector<std::string> files;
  DIR *dir = opendir(path.c_str());
  if (!dir)
    return files;

  struct dirent *entry;
  while ((entry = readdir(dir))) {
    if (std::strcmp(entry->d_name, ".") != 0 &&
        std::strcmp(entry->d_name, "..") != 0) {
      files.push_back(entry->d_name);
    }
  }
  closedir(dir);
  return files;
}

static std::string generateAutoindex(const std::string &dir_path,
                                     const std::string &url_path) 
{
  (void)dir_path; // not strictly needed here, but kept for future enhancements
  std::string base = url_path;
  if (base.empty() || base[base.size() - 1] != '/')
    base += "/";

  std::vector<std::string> items = getDirList(dir_path);
  std::string html = "<html><body><h1>Index of " + base + "</h1><ul>";
  for (size_t i = 0; i < items.size(); ++i) 
  {
    html += "<li><a href=\"" + base + items[i] + "\">" + items[i] + "</a></li>";
  }
  html += "</ul></body></html>";
  return html;
}

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

void Get::handle(const HTTPRequest &request, HttpResponse &response,
                     const LocationBlock &location)
{
    // // Match location
    // location : the location of the target. 
    const std::string target = request.getTarget();

    // check no target
    if (target.empty() || target[0] != '/') {
        setHtmlError(response, 400, "Bad Request", "Invalid request target.");
        return;
    }

    const std::string root = location.getRoot();
    const std::string filePath = joinPaths(root, target);
    const std::string dirPath = dirnameOf(filePath);

    // Resolves symlinks to avoid path traversals.
    // and store the real paths in the new variable. 
    char rootReal[PATH_MAX];
        if (realpath(root.c_str(), rootReal) == NULL) 
        {
            setHtmlError(response, 500, "Internal Server Error",
                        "Failed to resolve server root.");
            return;
        }

    char dirReal[PATH_MAX];
      if (realpath(dirPath.c_str(), dirReal) == NULL) 
      {
          setHtmlError(response, 404, "Not Found", "Target not found.");
      return;
    }

    // check the existance of the file 
    struct stat fileStat;
    if (lstat(filePath.c_str(), &fileStat) != 0) 
    {
      if (errno == ENOENT) 
        setHtmlError(response, 404, "Not Found", "Target not found.");
      else
        setHtmlError(response, 403, "Forbidden", "Cannot access target.");
      return;
    }
//-------------------------------------------------------------
    // Handle listing :

    if (S_ISDIR(fileStat.st_mode)) 
    {
      // Try serving an index file (location settings have priority)
      const std::string indexName = location.getIndex();
      std::string indexPath = joinPaths(filePath, indexName);
      struct stat idxStat;
      if (lstat(indexPath.c_str(), &idxStat) == 0 
          && S_ISREG(idxStat.st_mode) 
          && access(indexPath.c_str(), R_OK) == 0) 
      {
        std::ifstream idxFile(indexPath.c_str(), std::ios::binary);
        if (idxFile.is_open()) 
        {
          std::string content((std::istreambuf_iterator<char>(idxFile)),
                            std::istreambuf_iterator<char>());
          idxFile.close();
          response.setStatus(200, "OK");
          response.setBody(content);
          response.setHeader("Content-Type", "text/html");
          return;
        }
      }

      // If index doesn't exist, check if autoindex is enabled
      if (location.getAutoindex()) {
        std::string html = generateAutoindex(filePath, target);
        response.setStatus(200, "OK");
        response.setBody(html);
        response.setHeader("Content-Type", "text/html");
        return;
      }

      // Directory listing denied
      setHtmlError(response, 403, "Forbidden", "Directory listing denied.");
      return;
    }

    if (access(filePath.c_str(), R_OK) != 0) {
    setHtmlError(response, 403, "Forbidden", "Permission denied.");
    return;
    }

    // read the content of the file and return it 
    std::ifstream file(filePath.c_str(), std::ios::binary);// open the file in binary mode 

    if (!file.is_open()) {
        setHtmlError(response, 500, "Internal Server Error", "Failed to open file.");
        return;
    }
    // read all the data of the file till we find the EOF ( the second iterator is EOF)
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    response.setStatus(200, "OK");
    response.setBody(content);
    
    // Set Content-Type based on file extension
    std::string ext;
    size_t dotPos = filePath.rfind('.');
    if (dotPos != std::string::npos) {
        ext = filePath.substr(dotPos);
    }
    
    if (ext == ".html" || ext == ".htm")
        response.setHeader("Content-Type", "text/html");
    else if (ext == ".css")
        response.setHeader("Content-Type", "text/css");
    else if (ext == ".js")
        response.setHeader("Content-Type", "application/javascript");
    else if (ext == ".json")
        response.setHeader("Content-Type", "application/json");
    else if (ext == ".png")
        response.setHeader("Content-Type", "image/png");
    else if (ext == ".jpg" || ext == ".jpeg")
        response.setHeader("Content-Type", "image/jpeg");
    else if (ext == ".gif")
        response.setHeader("Content-Type", "image/gif");
    else if (ext == ".txt")
        response.setHeader("Content-Type", "text/plain");
    else
        response.setHeader("Content-Type", "application/octet-stream");
}




// 2 - Directory + no index + autoindex off → 403 Forbidden


// http://localhost:8080/index.esa?q=webserv&page=2&sort=asc
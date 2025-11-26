#include "ConfigurationFile.hpp"

#include <stdexcept>

ConfigurationFile::ConfigurationFile() : _filename(""), _size(0) {}

ConfigurationFile::ConfigurationFile(const std::string &filename)
    : _filename(filename), _size(0) {}

ConfigurationFile::ConfigurationFile(const ConfigurationFile &other)
    : _filename(other._filename), _size(other._size) {}

ConfigurationFile &
ConfigurationFile::operator=(const ConfigurationFile &other) {
  if (this != &other) {
    _filename = other._filename;
    _size = other._size;
  }
  return *this;
}

ConfigurationFile::~ConfigurationFile() {}

std::string ConfigurationFile::getFilename() const { return _filename; }

size_t ConfigurationFile::getSize() const { return _size; }

int ConfigurationFile::getTypePath(const std::string &path) {
  struct stat buffer;
  int res;

  res = stat(path.c_str(), &buffer);
  if (res == 0) {
    if (S_ISREG(buffer.st_mode))
      return 1; // Regular file
    else if (S_ISDIR(buffer.st_mode))
      return 2; // Directory
    else
      return 3; // Other types
  }
  return -1;
}

int ConfigurationFile::checkFile(const std::string &filepath, int mode) {
  int accessResult;

  // access() returns 0 if the requested access is permitted
  // , -1 otherwise (sets errno)
  accessResult = access(filepath.c_str(), mode);
  return accessResult;
}

int ConfigurationFile::doesFileExistAndIsReadable(const std::string &filepath,
                                                  const std::string &index) {
  if (getTypePath(index) == 1 && checkFile(index, 4) == 0)
    return (0);

  if (getTypePath(filepath + index) == 1 && checkFile(filepath + index, 4) == 0)
    return (0);

  std::string with_slash = filepath;
  if (!with_slash.empty() && with_slash[with_slash.size() - 1] != '/')
    with_slash += "/";
  with_slash += index;

  if (getTypePath(with_slash) == 1 && checkFile(with_slash, 4) == 0)
    return (0);

  return (-1);
}

std::string
ConfigurationFile::getFileContent(const std::string &filepath) const {
  std::ifstream fileStream(filepath.c_str());
  std::stringstream buffer;
  if (!fileStream.is_open()) {
    throw std::runtime_error("Could not open file: " +
                             filepath); // ToDo : To optimized later
  }
  buffer << fileStream.rdbuf();
  return buffer.str();
}

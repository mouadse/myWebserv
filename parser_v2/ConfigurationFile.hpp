#ifndef CONFIGURATIONFILE_HPP
#define CONFIGURATIONFILE_HPP

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

class ConfigurationFile {
private:
  std::string _filename;
  size_t _size;

public:
  ConfigurationFile();
  ConfigurationFile(const std::string &filename);
  ConfigurationFile(const ConfigurationFile &other);
  ConfigurationFile &operator=(const ConfigurationFile &other);
  ~ConfigurationFile();

  std::string getFilename() const;
  size_t getSize() const;

  // Utils functions
  static int getTypePath(const std::string &path);
  static int checkFile(const std::string &filepath, int mode);
  static int doesFileExistAndIsReadable(const std::string &filepath,
                                        const std::string &index);
  std::string getFileContent(const std::string &filepath) const;
};

#endif

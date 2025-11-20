#include "../include/ConfigFile.hpp"
#include "../include/ParserUtils.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

ConfigFile::ConfigFile() : _path(""), _size(0) {}

ConfigFile::ConfigFile(const std::string &path) : _path(path), _size(0) {}

ConfigFile::ConfigFile(const ConfigFile &other) : _path(other._path), _size(other._size) {}

ConfigFile &ConfigFile::operator=(const ConfigFile &rhs)
{
	if (this != &rhs)
	{
		_path = rhs._path;
		_size = rhs._size;
	}
	return (*this);
}

ConfigFile::~ConfigFile() {}

int ConfigFile::getTypePath(const std::string &path)
{
	struct stat	buffer;
	int			result;

	result = stat(path.c_str(), &buffer);
	if (result == 0)
	{
		if (buffer.st_mode & S_IFREG)
			return (1);
		else if (buffer.st_mode & S_IFDIR)
			return (2);
		else
			return (3);
	}
	return (-1);
}

int	ConfigFile::checkFile(const std::string &path, int mode)
{
	return (access(path.c_str(), mode));
}

int ConfigFile::isFileExistAndReadable(const std::string &path, const std::string &index)
{
	if (getTypePath(index) == 1 && checkFile(index, 4) == 0)
		return (0);
	if (getTypePath(path + index) == 1 && checkFile(path + index, 4) == 0)
		return (0);
	std::string with_slash = path;
	if (!with_slash.empty() && with_slash[with_slash.size() - 1] != '/')
		with_slash += "/";
	with_slash += index;
	if (getTypePath(with_slash) == 1 && checkFile(with_slash, 4) == 0)
		return (0);
	return (-1);
}

std::string	ConfigFile::readFile(const std::string &path) const
{
	std::ifstream config_file(path.c_str());
	if (!config_file.is_open())
		throw ConfigError("Cannot open config file: " + path);
	std::stringstream buffer;
	buffer << config_file.rdbuf();
	return (buffer.str());
}

std::string ConfigFile::getPath() const
{
	return (_path);
}

size_t ConfigFile::getSize() const
{
	return (_size);
}

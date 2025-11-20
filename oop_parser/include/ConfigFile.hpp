#ifndef CONFIGFILE_HPP
#define CONFIGFILE_HPP

#include <string>

class ConfigFile
{
	private:
		std::string		_path;
		size_t			_size;

	public:
		ConfigFile();
		explicit ConfigFile(const std::string &path);
		ConfigFile(const ConfigFile &other);
		ConfigFile &operator=(const ConfigFile &rhs);
		~ConfigFile();

		static int getTypePath(const std::string &path);
		static int checkFile(const std::string &path, int mode);
		static int isFileExistAndReadable(const std::string &path, const std::string &index);
		std::string	readFile(const std::string &path) const;

		std::string getPath() const;
		size_t getSize() const;
};

#endif

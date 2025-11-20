#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>
#include <map>

#include "ConfigFile.hpp"
#include "ParserUtils.hpp"

class Location
{
	private:
		std::string					_path;
		std::string					_root;
		bool						_autoindex;
		std::string					_index;
		std::vector<short>			_methods;
		std::string					_return;
		std::string					_alias;
		std::vector<std::string>	_cgi_path;
		std::vector<std::string>	_cgi_ext;
		unsigned long				_client_max_body_size;

	public:
		std::map<std::string, std::string> _ext_path;

		Location();
		Location(const Location &other);
		Location &operator=(const Location &rhs);
		~Location();

		void setPath(const std::string &parametr);
		void setRootLocation(const std::string &parametr);
		void setMethods(const std::vector<std::string> &methods);
		void setAutoindex(const std::string &parametr);
		void setIndexLocation(const std::string &parametr);
		void setReturn(const std::string &parametr);
		void setAlias(const std::string &parametr);
		void setCgiPath(const std::vector<std::string> &path);
		void setCgiExtension(const std::vector<std::string> &extension);
		void setMaxBodySize(const std::string &parametr);
		void setMaxBodySize(unsigned long parametr);

		const std::string &getPath() const;
		const std::string &getRootLocation() const;
		const std::vector<short> &getMethods() const;
		const bool &getAutoindex() const;
		const std::string &getIndexLocation() const;
		const std::string &getReturn() const;
		const std::string &getAlias() const;
		const std::vector<std::string> &getCgiPath() const;
		const std::vector<std::string> &getCgiExtension() const;
		const std::map<std::string, std::string> &getExtensionPath() const;
		const unsigned long &getMaxBodySize() const;

		std::string getPrintMethods() const;
};

#endif

#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>
#include <map>

#include "ConfigFile.hpp"
#include "ParserUtils.hpp"

/**
 * @class Location
 * @brief Stores configuration for a specific location block.
 *
 * This class handles settings for a specific URL path (location) within a server.
 * It includes allowed methods, root directory, redirection, CGI settings, etc.
 */
class Location
{
	private:
		std::string					_path;                  ///< Location path (e.g., / or /images)
		std::string					_root;                  ///< Root directory for this location
		bool						_autoindex;             ///< Autoindex enabled/disabled
		std::string					_index;                 ///< Default index file
		std::vector<short>			_methods;               ///< Allowed HTTP methods (GET, POST, etc.)
		std::string					_return;                ///< Redirection URL
		std::string					_alias;                 ///< Alias path
		std::vector<std::string>	_cgi_path;              ///< Paths to CGI executables
		std::vector<std::string>	_cgi_ext;               ///< CGI extensions (.py, .php)
		unsigned long				_client_max_body_size;  ///< Max client body size for this location

	public:
		std::map<std::string, std::string> _ext_path;       ///< Map of extension to CGI executable path

		Location();
		Location(const Location &other);
		Location &operator=(const Location &rhs);
		~Location();

		// Setters
		void setPath(const std::string &parameter);
		void setRootLocation(const std::string &parameter);
		void setMethods(const std::vector<std::string> &methods);
		void setAutoindex(const std::string &parameter);
		void setIndexLocation(const std::string &parameter);
		void setReturn(const std::string &parameter);
		void setAlias(const std::string &parameter);
		void setCgiPath(const std::vector<std::string> &path);
		void setCgiExtension(const std::vector<std::string> &extension);
		void setMaxBodySize(const std::string &parameter);
		void setMaxBodySize(unsigned long parameter);

		// Getters
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

		/**
		 * @brief Returns a string representation of allowed methods.
		 *
		 * @return std::string (e.g., "GET, POST")
		 */
		std::string getPrintMethods() const;
};

#endif

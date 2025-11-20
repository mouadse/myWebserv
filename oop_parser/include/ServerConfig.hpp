#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <map>
#include <vector>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>

#include "Location.hpp"
#include "ConfigFile.hpp"
#include "ParserUtils.hpp"

/**
 * @class ServerConfig
 * @brief Stores configuration for a single server block.
 *
 * This class holds all the settings for a server, such as port, host,
 * server name, root directory, error pages, and a list of locations.
 * It also provides methods to validate and set these values.
 */
class ServerConfig
{
	private:
		uint16_t						_port;                  ///< Listening port
		in_addr_t						_host;                  ///< Host address (IP)
		std::string						_server_name;           ///< Server name
		std::string						_root;                  ///< Root directory
		unsigned long					_client_max_body_size;  ///< Max client body size
		std::string						_index;                 ///< Default index file
		bool							_autoindex;             ///< Autoindex enabled/disabled
		std::map<short, std::string>	_error_pages;           ///< Custom error pages
		std::vector<Location> 			_locations;             ///< List of location blocks
		struct sockaddr_in 				_server_address;        ///< Socket address structure
		int     						_listen_fd;             ///< Listening file descriptor

	public:
		ServerConfig();
		~ServerConfig();
		ServerConfig(const ServerConfig &other);
		ServerConfig &operator=(const ServerConfig & rhs);

		/**
		 * @brief Initializes default error pages.
		 */
		void initErrorPages(void);

		// Setters
		void setServerName(std::string server_name);
		void setHost(std::string parameter);
		void setRoot(std::string root);
		void setFd(int fd);
		void setPort(std::string parameter);
		void setClientMaxBodySize(std::string parameter);
		void setErrorPages(std::vector<std::string> parameter);
		void setIndex(std::string index);

		/**
		 * @brief Sets a new location block.
		 *
		 * @param nameLocation The path of the location.
		 * @param parameter The list of tokens inside the location block.
		 */
		void setLocation(std::string nameLocation, std::vector<std::string> parameter);
		void setAutoindex(std::string autoindex);

		// Validators
		bool isValidHost(std::string host) const;
		bool isValidErrorPages();

		/**
		 * @brief Validates a location object.
		 *
		 * @param location The location to validate.
		 * @return 0 if valid, error code otherwise.
		 */
		int isValidLocation(Location &location) const;

		// Getters
		const std::string &getServerName() const;
		const uint16_t &getPort() const;
		const in_addr_t &getHost() const;
		const size_t &getClientMaxBodySize() const;
		const std::vector<Location> &getLocations() const;
		const std::string &getRoot() const;
		const std::map<short, std::string> &getErrorPages() const;
		const std::string &getIndex() const;
		const bool &getAutoindex() const;
		const std::string &getPathErrorPage(short key) const;
		std::vector<Location>::const_iterator getLocationKey(const std::string &key) const;

		static void checkToken(std::string &parameter);
		bool checkLocaitons() const;

		/**
		 * @brief Sets up the server socket (socket, bind).
		 */
		void	setupServer();
		int     getFd() const;
};

#endif

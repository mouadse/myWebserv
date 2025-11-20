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

class ServerConfig
{
	private:
		uint16_t						_port;
		in_addr_t						_host;
		std::string						_server_name;
		std::string						_root;
		unsigned long					_client_max_body_size;
		std::string						_index;
		bool							_autoindex;
		std::map<short, std::string>	_error_pages;
		std::vector<Location> 			_locations;
		struct sockaddr_in 				_server_address;
		int     						_listen_fd;

	public:
		ServerConfig();
		~ServerConfig();
		ServerConfig(const ServerConfig &other);
		ServerConfig &operator=(const ServerConfig & rhs);

		void initErrorPages(void);

		void setServerName(std::string server_name);
		void setHost(std::string parametr);
		void setRoot(std::string root);
		void setFd(int fd);
		void setPort(std::string parametr);
		void setClientMaxBodySize(std::string parametr);
		void setErrorPages(std::vector<std::string> parametr);
		void setIndex(std::string index);
		void setLocation(std::string nameLocation, std::vector<std::string> parametr);
		void setAutoindex(std::string autoindex);

		bool isValidHost(std::string host) const;
		bool isValidErrorPages();
		int isValidLocation(Location &location) const;

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

		static void checkToken(std::string &parametr);
		bool checkLocaitons() const;

		void	setupServer();
		int     getFd() const;
};

#endif

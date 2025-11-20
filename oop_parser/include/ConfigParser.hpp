#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <vector>
#include <string>

#include "ServerConfig.hpp"

class ConfigParser
{
	private:
		std::vector<ServerConfig>	_servers;
		std::vector<std::string>	_server_config;
		size_t						_nb_server;

	public:
		ConfigParser();
		~ConfigParser();
		ConfigParser(const ConfigParser &other);
		ConfigParser &operator=(const ConfigParser &rhs);

		int createCluster(const std::string &config_file);

		void splitServers(std::string &content);
		void removeComments(std::string &content);
		void removeWhiteSpace(std::string &content);
		size_t findStartServer(size_t start, std::string &content);
		size_t findEndServer(size_t start, std::string &content);
		void createServer(std::string &config, ServerConfig &server);
		void checkServers();
		std::vector<ServerConfig>	getServers() const;

		int print(std::ostream &out) const;
};

#endif

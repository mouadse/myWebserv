#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <vector>
#include <string>

#include "ServerConfig.hpp"

/**
 * @class ConfigParser
 * @brief Handles the parsing of the configuration file.
 *
 * This class is responsible for reading the configuration file, cleaning it up
 * (removing comments and whitespace), splitting it into server blocks, and then
 * parsing each server block into a ServerConfig object.
 */
class ConfigParser
{
	private:
		std::vector<ServerConfig>	_servers;       ///< List of parsed server configurations
		std::vector<std::string>	_server_config; ///< List of raw server block strings
		size_t						_nb_server;     ///< Number of server blocks found

		/**
		 * @brief Parses the content of a single server block.
		 *
		 * @param config The string content of the server block.
		 * @param server The ServerConfig object to populate.
		 */
		void _parseServerContent(const std::string &config, ServerConfig &server);

		/**
		 * @brief Parses a location block within a server.
		 *
		 * @param tokens The list of tokens in the configuration.
		 * @param index The current index in the tokens vector.
		 * @param server The ServerConfig object to populate.
		 */
		void _parseLocation(const std::vector<std::string> &tokens, size_t &index, ServerConfig &server);

	public:
		ConfigParser();
		~ConfigParser();
		ConfigParser(const ConfigParser &other);
		ConfigParser &operator=(const ConfigParser &rhs);

		/**
		 * @brief Main entry point to parse the configuration file.
		 *
		 * @param config_file Path to the configuration file.
		 * @return 0 on success.
		 * @throw ConfigError on failure.
		 */
		int createCluster(const std::string &config_file);

		/**
		 * @brief Splits the configuration content into individual server blocks.
		 *
		 * @param content The full configuration file content.
		 */
		void splitServers(std::string &content);

		/**
		 * @brief Removes comments (lines starting with #) from the content.
		 *
		 * @param content The configuration content.
		 */
		void removeComments(std::string &content);

		/**
		 * @brief Removes leading and trailing whitespace from the content.
		 *
		 * @param content The configuration content.
		 */
		void removeWhiteSpace(std::string &content);

		/**
		 * @brief Finds the start index of a server block.
		 *
		 * @param start The index to start searching from.
		 * @param content The content to search.
		 * @return The index where the server block starts.
		 */
		size_t findStartServer(size_t start, std::string &content);

		/**
		 * @brief Finds the end index of a server block (closing brace).
		 *
		 * @param start The index to start searching from (after the opening brace).
		 * @param content The content to search.
		 * @return The index of the closing brace.
		 */
		size_t findEndServer(size_t start, std::string &content);

		/**
		 * @brief Creates a ServerConfig object from a configuration string.
		 *
		 * @param config The configuration string for the server.
		 * @param server The ServerConfig object to populate.
		 */
		void createServer(std::string &config, ServerConfig &server);

		/**
		 * @brief Validates the parsed servers (e.g., checking for duplicate ports/hosts).
		 */
		void checkServers();

		/**
		 * @brief Returns the list of parsed server configurations.
		 *
		 * @return A vector of ServerConfig objects.
		 */
		std::vector<ServerConfig>	getServers() const;

		/**
		 * @brief Prints the configuration to the output stream.
		 *
		 * @param out The output stream.
		 * @return 0 on success.
		 */
		int print(std::ostream &out) const;
};

#endif

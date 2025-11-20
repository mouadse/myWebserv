#ifndef PARSER_UTILS_HPP
#define PARSER_UTILS_HPP

#include <string>
#include <stdexcept>
#include <limits>

static const unsigned long kDefaultMaxBodySize = 30000000UL;

class ConfigError : public std::runtime_error
{
	public:
		explicit ConfigError(const std::string &message);
};

int ft_stoi(const std::string &str);
unsigned int fromHexToDec(const std::string &nb);
std::string statusCodeString(short statusCode);
void requireTrailingSemicolon(std::string &token, const std::string &context);
bool isDigits(const std::string &value);
std::string trim(const std::string &value);

#endif

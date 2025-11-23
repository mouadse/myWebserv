#ifndef PARSERUTILS_HPP
#define PARSERUTILS_HPP
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

static const unsigned long kDefaultMaxBodySize = 30000000UL; // 30 MB default

bool isAllDigits(const std::string &value);
int stoiStrict(const std::string &str);
unsigned int hexToUint(const std::string &hex);
std::string statusCodeToString(short statusCode);
std::string trimWhitespace(const std::string &value);
void enforceTrailingSemicolon(std::string &token, const std::string &context);

#endif

#include "ParserUtils.hpp"

ParserUtils::ParserUtils() {}

ParserUtils::ParserUtils(const ParserUtils &other) {}

ParserUtils &ParserUtils::operator=(const ParserUtils &other) { return *this; }

ParserUtils::~ParserUtils() {}

bool ParserUtils::isAllDigits(const std::string &value) {
  for (size_t i = 0; i < value.length(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(value[i]))) {
      return false;
    }
  }
  return true;
}

int ParserUtils::stoiStrict(const std::string &str) {
  if (!isAllDigits(str)) {
    throw std::invalid_argument("Invalid integer string: " + str);
  }
  std::stringstream ss(str);
  long value = 0;
  ss >> value;

  if (ss.fail() || value > std::numeric_limits<int>::max()) {
    throw std::invalid_argument("Value is out of bounds: " + str);
  }
  return (static_cast<int>(value));
}

unsigned int ParserUtils::hexToUint(const std::string &hex) {
  unsigned int res = 0;
  std::stringstream ss;
  ss << std::hex << hex;
  ss >> res;
  if (ss.fail()) {
    throw std::invalid_argument("Invalid hexadecimal string: " + hex);
  }
  return res;
}

std::string ParserUtils::trimWhitespace(const std::string &value) {
  const std::string whitespace = " \t\n\r\f\v";
  if (value.empty()) {
    return value;
  }
  size_t start = value.find_first_not_of(whitespace);
  if (start == std::string::npos) {
    return "";
  }
  size_t end = value.find_last_not_of(whitespace);
  return value.substr(start, end - start + 1);
}
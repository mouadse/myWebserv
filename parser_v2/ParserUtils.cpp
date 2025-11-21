#include "ParserUtils.hpp"

ParserUtils::ParserUtils() {}

ParserUtils::ParserUtils(const ParserUtils &other) {}

ParserUtils &ParserUtils::operator=(const ParserUtils &other) { return *this; }

ParserUtils::~ParserUtils() {}

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
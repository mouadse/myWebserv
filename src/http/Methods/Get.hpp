#ifndef GET_HPP
#define GET_HPP

#include "../../config/LocationBlock.hpp"
#include "../HTTPRequest.hpp"
#include "../HTTPResponse.hpp"

class Get {
public:
  static void handle(const HTTPRequest &request, HttpResponse &response,
                     const LocationBlock &location);
};

#endif

#ifndef DELETE_HPP
#define DELETE_HPP

#include "../../config/LocationBlock.hpp"
#include "../HTTPRequest.hpp"
#include "../HTTPResponse.hpp"

class Delete {
public:
  static void handle(const HTTPRequest &request, HttpResponse &response,
                     const LocationBlock &location);
};

#endif
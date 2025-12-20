#ifndef GET_HPP
#define GET_HPP

#include "../HTTPRequest.hpp"
#include "../HTTPResponse.hpp"
#include "../../config/LocationBlock.hpp"

class Get {
public:
    static void handle(const HTTPRequest &request, HttpResponse &response, const LocationBlock &location);
};

#endif

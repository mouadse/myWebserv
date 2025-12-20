#ifndef POST_HPP
#define POST_HPP

#include "../HTTPRequest.hpp"
#include "../HTTPResponse.hpp"
#include "../../config/LocationBlock.hpp"

class Post {
public:
    static void handle(const HTTPRequest &request, HttpResponse &response, const LocationBlock &location);
};

#endif

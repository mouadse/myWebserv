#ifndef POST_HPP
#define POST_HPP

#include "config/LocationBlock.hpp"
#include "../HTTPRequest.hpp"
#include "../HTTPResponse.hpp"
#include "MultipartParser.hpp"

class POST
{
    private : 

    public :
    static void handle(const HTTPRequest &request, HttpResponse &response,
                     const LocationBlock &location);
};

#endif

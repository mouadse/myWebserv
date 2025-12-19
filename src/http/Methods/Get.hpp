#ifndef GET_HPP
#define GET_HPP

#include "../../config/LocationBlock.hpp"
#include "../HTTPRequest.hpp"
#include "../HTTPResponse.hpp"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <limits.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

class Get
{
    private : 

    public :
    static void handle(const HTTPRequest &request, HttpResponse &response,
                     const LocationBlock &location);
};

#endif

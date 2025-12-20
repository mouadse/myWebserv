/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: webserv <webserv@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 13:49:56 by webserv          #+#    #+#             */
/*   Updated: 2025/12/17 19:22:06 by webserv           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP
#include "../config/LocationBlock.hpp"
#include "../config/WebserverConfig.hpp"
#include "../config/LocationBlock.hpp" 
#include "HTTPResponse.hpp"
#include "HTTPRequest.hpp"
// #include "MultipartParser.hpp"
#include "Methods/Delete.hpp"
#include "Methods/Get.hpp"
#include "Methods/Post.hpp"
#include <string>
#include <map>
#include <vector>
#include <ctime>
#include <sstream>
#include <unistd.h>

class RequestHandler {
    private:
        void handleGET(const HTTPRequest &request, HttpResponse &response);
        void handlePOST(const HTTPRequest &request, HttpResponse &response);
        void handleDELETE(const HTTPRequest &request, HttpResponse &response);
        void handleCGI(const HTTPRequest &request, HttpResponse &response);
        void setCommonHeaders(HttpResponse &response);
        void handleError(int errorCode, const std::string &errorMessage, HttpResponse &response);
        void respondMethodNotAllowed(const LocationBlock &location,
                                     HttpResponse &response);

        void handleMultipartPOST(const HTTPRequest &request, const LocationBlock &loc,  HttpResponse &response);
        void handleUrlEncodedPOST(const HTTPRequest &request, const LocationBlock &loc, HttpResponse &response);
        void handleRawPOST(const HTTPRequest &request, const LocationBlock &loc, HttpResponse &response);
        
        
        std::string generateUniqueFilename(const std::string &root, const std::string &originalName);
        const WebserverConfig &server;
        std::string sanitizeFilename(std::string name);
    public:
        RequestHandler();
        RequestHandler(const WebserverConfig &srv);
        void process(const HTTPRequest &request, HttpResponse &response);
        const LocationBlock &matchLocation(const std::string &path);
};

#endif
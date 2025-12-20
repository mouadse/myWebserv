/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 13:49:56 by webserv          #+#    #+#             */
/*   Updated: 2025/12/20 21:47:08 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP
#include "../config/LocationBlock.hpp"
#include "../config/WebserverConfig.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "Methods/Delete.hpp"
#include "Methods/Get.hpp"
#include "Methods/Post.hpp"
#include <ctime>
#include <map>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

class RequestHandler {
private:
  void handleGET(const HTTPRequest &request, HttpResponse &response);
  void handlePOST(const HTTPRequest &request, HttpResponse &response);
  void handleDELETE(const HTTPRequest &request, HttpResponse &response);
  void handleCGI(const HTTPRequest &request, HttpResponse &response);
  void setCommonHeaders(HttpResponse &response);
  void handleError(int errorCode, const std::string &errorMessage,
                   HttpResponse &response);
  void respondMethodNotAllowed(const LocationBlock &location,
                               HttpResponse &response);

  const WebserverConfig &server;
  std::string sanitizeFilename(std::string name);

public:
  RequestHandler();
  RequestHandler(const WebserverConfig &srv);
  void process(const HTTPRequest &request, HttpResponse &response);
  const LocationBlock &matchLocation(const std::string &path);
};

#endif
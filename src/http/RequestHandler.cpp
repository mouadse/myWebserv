/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/02 13:49:45 by webserv          #+#    #+#             */

/*   Updated: 2025/12/13 21:55:30 by webserv           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include "../cgi/CgiHandler.hpp"
#include "../utils/PathUtils.hpp"
#include "Methods/Delete.hpp"
#include "Methods/Get.hpp"
#include "Methods/Post.hpp"

namespace {
std::string getExtension(const std::string &path) {
  std::string::size_type slash = path.find_last_of('/');
  std::string::size_type dot = path.find_last_of('.');
  if (dot == std::string::npos)
    return "";
  if (slash != std::string::npos && dot < slash)
    return "";
  return path.substr(dot);
}
bool isAdminPath(const std::string &target) {
  return target == "/admin" || target.compare(0, 7, "/admin/") == 0;
}

std::string buildAllowHeader(const std::vector<short> &methods) {
  static const char *names[] = {"GET", "POST", "DELETE", "PUT", "HEAD"};
  std::string result;
  for (size_t i = 0; i < methods.size() && i < 5; ++i) {
    if (!methods[i])
      continue;
    if (!result.empty())
      result += ", ";
    result += names[i];
  }
  return result;
}
} // namespace

RequestHandler::RequestHandler(const WebserverConfig &srv) : server(srv) {}

void RequestHandler::respondMethodNotAllowed(const LocationBlock &location,
                                             HttpResponse &response) {
  const std::vector<short> &methods = location.getMethods();
  std::string allow = buildAllowHeader(methods);
  if (!allow.empty())
    response.setHeader("Allow", allow);
  handleError(405, "Method Not Allowed", response);
}

void RequestHandler::process(const HTTPRequest &request,
                             HttpResponse &response) {
  response = HttpResponse();
  response.setBody("");
  const std::string method = request.getMethod();
  const std::string target = request.getTarget();
  const LocationBlock &location = matchLocation(target);
  bool isCGI = !location.getExtensionToCgiMap().empty();
  setCommonHeaders(response);
  if (request.hasError()) {
    const std::string &msg = request.getErrorMessage();
    int code = 400;
    if (msg == "Length Required")
      code = 411;
    else if (msg == "Payload Too Large")
      code = 413;
    else if (msg == "URI Too Long")
      code = 414;
    else if (msg == "Request Header Fields Too Large")
      code = 431;
    else if (msg == "Method Not Allowed")
      code = 405;
    else if (msg == "Unsupported HTTP version")
      code = 505;
    handleError(code, msg, response);
    return;
  }

  if (isAdminPath(target) && request.getHeader("Authorization").empty()) {
    response.setHeader("WWW-Authenticate", "Basic realm=\"webserv\"");
    handleError(401, "Unauthorized", response);
    return;
  }

  bool stripBody = false;
  if (method == "HEAD") {
    stripBody = true;
  }

  if (method == "GET" || method == "HEAD") {
    if (isCGI)
      handleCGI(request, response);
    else
      handleGET(request, response);
  } else if (method == "POST") {
    if (isCGI)
      handleCGI(request, response);
    else
      handlePOST(request, response);
  } else if (method == "DELETE") {
    handleDELETE(request, response);
  } else {
    handleError(405, "Method Not Allowed", response);
  }

  if (stripBody) {
    std::string originalLength = response.getHeader("Content-Length");
    response.setBody("");
    response.setHeader("Content-Length", originalLength);
  }
}

void RequestHandler::setCommonHeaders(HttpResponse &response) {
  response.setHeader("Server", "SimpleWebServer/1.0");
  std::time_t now = std::time(NULL);
  char dateBuffer[100];
  std::tm *gmt = std::gmtime(&now);
  std::strftime(dateBuffer, sizeof(dateBuffer), "%a, %d %b %Y %H:%M:%S GMT",
                gmt);
  response.setHeader("Date", dateBuffer);
  response.setHeader("Connection", "keep-alive");
}

void RequestHandler::handleGET(const HTTPRequest &request,
                               HttpResponse &response) {
  LocationBlock location;
  location = matchLocation(request.getTarget());
  const std::vector<short> locMethods = location.getMethods();

  if (locMethods.size() < 1 || locMethods[0] == 0) {
    respondMethodNotAllowed(location, response);
    return;
  }

  Get::handle(request, response, location);
}

void RequestHandler::handlePOST(const HTTPRequest &request,
                                HttpResponse &response) {
  LocationBlock location;
  location = matchLocation(request.getTarget());
  const std::vector<short> locMethods = location.getMethods();

  if (locMethods.size() < 2 || locMethods[1] == 0) {
    respondMethodNotAllowed(location, response);
    return;
  }
  unsigned long limit = location.getMaxBodySize();
  if (request.getBody().size() > limit) {
    handleError(413, "Payload Too Large", response);
    return;
  }
  std::string ct = request.getHeader("Content-Type");

  if (ct.empty()) {
    response.setStatus(400, "Bad Request");
    response.setBody("Missing Content-Type");
    return;
  }

  Post::handle(request, response, location);
}

void RequestHandler::handleCGI(const HTTPRequest &request,
                               HttpResponse &response) {
  const LocationBlock &location = matchLocation(request.getTarget());
  const std::map<std::string, std::string> &cgiMap =
      location.getExtensionToCgiMap();
  if (cgiMap.empty()) {
    handleError(404, "Not Found", response);
    return;
  }

  std::string scriptUri = request.getTarget();
  if (scriptUri == location.getPath() ||
      (!scriptUri.empty() && scriptUri[scriptUri.size() - 1] == '/')) {
    if (location.getIndex().empty()) {
      handleError(404, "Not Found", response);
      return;
    }
    scriptUri = joinPaths(location.getPath(), location.getIndex());
  }

  const std::string ext = getExtension(scriptUri);
  std::map<std::string, std::string>::const_iterator it = cgiMap.find(ext);
  if (ext.empty() || it == cgiMap.end()) {
    handleError(404, "Not Found", response);
    return;
  }

  const std::string scriptPath = joinPaths(location.getRoot(), scriptUri);
  struct stat st;
  if (stat(scriptPath.c_str(), &st) != 0) {
    if (errno == ENOENT)
      handleError(404, "Not Found", response);
    else
      handleError(403, "Forbidden", response);
    return;
  }
  if (S_ISDIR(st.st_mode)) {
    handleError(404, "Not Found", response);
    return;
  }
  if (access(scriptPath.c_str(), R_OK) != 0) {
    handleError(403, "Forbidden", response);
    return;
  }
  if (!it->second.empty() && access(it->second.c_str(), X_OK) != 0) {
    handleError(500, "Internal Server Error", response);
    return;
  }

  CgiHandler cgi(request, scriptPath, scriptUri, it->second);
  cgi.execute(response);
}

void RequestHandler::handleDELETE(const HTTPRequest &request,
                                  HttpResponse &response) {
  LocationBlock location;
  location = matchLocation(request.getTarget());
  const std::vector<short> locMethods = location.getMethods();

  if (locMethods.size() < 3 || locMethods[2] == 0) {
    respondMethodNotAllowed(location, response);
    return;
  }

  Delete::handle(request, response, location);
}

void RequestHandler::handleError(int errorCode, const std::string &errorMessage,
                                 HttpResponse &response) {

  response.setStatus(errorCode, errorMessage);
  std::stringstream body;
  body << "<html><head><title>" << errorCode << " " << errorMessage
       << "</title></head>"
       << "<body><h1>" << errorCode << " " << errorMessage << "</h1>"
       << "<p>The request could not be processed.</p>"
       << "</body></html>";

  response.setBody(body.str());
  response.setHeader("Content-Type", "text/html");
}

const LocationBlock &RequestHandler::matchLocation(const std::string &path) {
  const std::vector<LocationBlock> &locs = server.getLocationBlocks();
  const LocationBlock *best = NULL;
  size_t best_len = 0;
  for (size_t i = 0; i < locs.size(); i++) {
    const std::string &locPath = locs[i].getPath();

    if (path.compare(0, locPath.size(), locPath) == 0) {
      if (locPath.size() > best_len) {
        best_len = locPath.size();
        best = &locs[i];
      }
    }
  }
  if (best)
    return *best;
  static LocationBlock defaultLoc;
  defaultLoc.setPath("/");
  defaultLoc.setRoot(server.getRoot());
  defaultLoc.setAutoindex(server.getAutoindex() ? "on" : "off");
  defaultLoc.setIndex(server.getIndex());
  defaultLoc.setMaxBodySize(server.getMaxBodySize());
  return (defaultLoc);
}

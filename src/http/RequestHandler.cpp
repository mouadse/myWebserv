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
#include <stdexcept>

RequestHandler::RequestHandler(const WebserverConfig &srv) : server(srv) {}

void RequestHandler::process(const HTTPRequest &request,
                             HttpResponse &response) {
  // Fixed bug:
  // Initialize response object for each request to avoid residual data
  // from previous requests that could lead to incorrect responses.
  response = HttpResponse();
  response.setBody("");
  bool isCGI = false;
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
  const std::string method = request.getMethod();
  const std::string target = request.getTarget();
  if (target.size() >= 4 && target.substr(target.size() - 4) == ".php")
    isCGI = true;
  else if (target.size() >= 3 && target.substr(target.size() - 3) == ".py")
    isCGI = true;
  if (method == "GET") {
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


void RequestHandler::handlePOST(const HTTPRequest &request, HttpResponse &response)
{


    LocationBlock location;
    location = matchLocation(request.getTarget());
    const std::vector<short> locMethods = location.getMethods();
    
    if (locMethods[1] == 0)
    {
        std::string allow;
        response.setStatus(405, "Method Not Allowed");
        if (locMethods[0])
            allow += "GET, ";
        if (locMethods[2])
            allow += "DELETE, ";
        if (locMethods[3])
            allow += "PUT, ";
        if (locMethods[4])
            allow += "HEAD, ";
        if (!allow.empty())
            allow.erase(allow.size() - 2);
        response.setHeader("Allow", allow);
        response.setBody("<html><body><h1>Method Not Allowed</h1></body></html>");
        response.setHeader("Content-Type", "text/html");
        return;
    }


  POST::handle(request, response, location);
    // unsigned long limit = location.getMaxBodySize();
    // if (request.getBody().size() > limit)
    // {
    //     handleError(413, "Payload Too Large", response);
    //     return;
    // }
    // std::string ct = request.getHeader("Content-Type");

    // std::string ctLower = ct;
    // for (size_t i = 0; i < ctLower.size(); i++)
    //     ctLower[i] = tolower(ctLower[i]);

    // if (ctLower.find("multipart/form-data") == std::string::npos)
    // {
    //     response.setStatus(415, "Unsupported Media Type");
    //     return;
    // }

    // if (ct.empty())
    // {
    //     response.setStatus(400, "Bad Request");
    //     response.setBody("Missing Content-Type");
    //     return;
    // }

    // if (ct.find("multipart/form-data") == std::string::npos)
    // {
    //     response.setStatus(415, "Unsupported Media Type");
    //     response.setBody("POST supports only multipart/form-data");
    //     response.setHeader("Content-Type", "text/html");
    //     return;
    // }
    // size_t pos = ct.find("boundary=");
    // if (pos == std::string::npos)
    // {
    //     response.setStatus(400, "Bad Request");
    //     response.setBody("Missing multipart boundary");
    //     return;
    // }

    // const std::string boundary = "--" + ct.substr(pos + 9);
    // if (boundary.empty())
    // {
    //     response.setStatus(400, "Bad Request");
    //     response.setBody("Empty boundary");
    //     return;
    // }
    // try {
    //     MultipartParser parser(boundary, request.getBody());

    //     while (parser.hasNextPart())
    //     {
    //         Part part = parser.nextPart();
    //         if (part.filename.empty())
    //         {
    //             response.setStatus(400, "Bad Request");
    //             return;
    //         }

    //         try
    //         {
    //             std::string filename = sanitizeFilename(part.filename);
                
    //             if (filename.empty())
    //             {
    //                 response.setStatus(400, "Bad Request");
    //                 return;
    //             }
    //             std::string root = location.getRoot();
    //             if (!root.empty() && root[root.size() - 1] == '/')
    //                 root.erase(root.size() - 1);

    //             std::string target = request.getTarget();
    //             if (target.empty() || target[0] != '/')
    //             {
    //                 response.setStatus(400, "Bad Request");
    //                 return;
    //             }
    //             if (target.find("..") != std::string::npos)
    //             {
    //                 response.setStatus(400, "Bad Request");
    //                 return;
    //             }


    //             std::string basePath =  root + request.getTarget();
    //             if (!basePath.empty() && basePath[basePath.size() - 1] != '/')
    //               basePath += '/';
    //             std::string path = basePath + filename;

    //             struct stat st;
    //             if (stat(basePath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
    //             {
    //                 response.setStatus(403, "Not Found");
    //                 return;
    //             }

    //             if (access(basePath.c_str(), W_OK) != 0)
    //             {
    //                 handleError(404, "Forbidden: Upload directory is not writable", response);
    //                 return;
    //             }
    //             std::cout << "Path = " << path << std::endl;
    //             int i = 1;
    //           std::string base = filename;
    //           while (access(path.c_str(), F_OK) == 0)
    //           {
    //             std::ostringstream oss;
    //             oss << i++;
    //             filename = oss.str() + "_" + base;
    //             path = basePath + filename;
    //           }


    //             std::ofstream file(path.c_str(), std::ios::binary);
    //             if (!file.is_open())
    //             {
    //                 handleError(500, "Internal Server Error", response);
    //                 return;
    //             }
    //             file.write(part.data.data(), part.data.size());
    //             file.close();
    //         }
    //         catch (...)
    //         {
    //             if (errno == EFBIG)
    //                 response.setStatus(413, "Payload Too Large");
    //             else
    //                 response.setStatus(400, "Bad Request");
    //             return;
    //         }

    //     }

    //     response.setStatus(201, "Created");
    //     response.setBody("Upload successful");
    //   }
    //   catch (...) {
    //       response.setStatus(400, "Bad Request");
    //       response.setBody("Invalid multipart body");
    //       return;
    //   }
    //   return;
}


void RequestHandler::handleCGI(const HTTPRequest &request, HttpResponse &response) {
  (void)request;
  (void)response;
}

void RequestHandler::handleDELETE(const HTTPRequest &request, HttpResponse &response) {
  LocationBlock location;
  location = matchLocation(request.getTarget());
  const std::vector<short> locMethods = location.getMethods();

  if (locMethods.size() < 3 || locMethods[2] == 0) {
    response.setStatus(405, "Method Not Allowed");
    std::string allow = location.getPrintMethods();
    if (!allow.empty())
      response.setHeader("Allow", allow);
    response.setBody("<html><body><h1>Method Not Allowed</h1></body></html>");
    response.setHeader("Content-Type", "text/html");
    return;
  }

  Delete::handle(request, response, location);
}


void RequestHandler::handleGET(const HTTPRequest &request,
                               HttpResponse &response) {
  LocationBlock location;
  location = matchLocation(request.getTarget());
  const std::vector<short> locMethods = location.getMethods();
  const std::string &ret = location.getReturn();
  if (!ret.empty()) {
    response.setStatus(302, "Found");
    response.setHeader("Location", ret);
    response.setHeader("Content-Type", "text/html");
    std::stringstream body;
    body << "<html><body><h1>Found</h1><a href=\"" << ret << "\">" << ret << "</a></body></html>";
    response.setBody(body.str());
    return;
  }

  if (locMethods.size() < 1 || locMethods[0] == 0) 
  {
    response.setStatus(405, "Method Not Allowed");
    std::string allow = location.getPrintMethods();
    if (!allow.empty())
      response.setHeader("Allow", allow);

    response.setBody("<html><body><h1>Method Not Allowed</h1></body></html>");
    response.setHeader("Content-Type", "text/html");
    return;
  }

  Get::handle(request, response, location);
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

std::string RequestHandler::sanitizeFilename(std::string name)
{
    size_t p = name.find_last_of("/\\");
    if (p != std::string::npos)
        name = name.substr(p + 1);
    if (name.size() > 255)
    {
        errno = EFBIG;
        throw std::runtime_error("filename too long");
    }

    if (name.empty() || name == "." || name == ".." || name[0] == '.')
        throw std::runtime_error("bad filename");

    std::string clean;
    for (size_t i = 0; i < name.size(); i++)
    {
        if (isalnum(name[i]) || name[i] == '.' || name[i] == '_' || name[i] == '-')
            clean += name[i];
    }
    if (clean.empty())
        throw std::runtime_error("bad filename");
    name = clean;


    return name;
}

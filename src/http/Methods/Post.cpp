#include "Post.hpp"



static void handleError(int errorCode, const std::string &errorMessage,
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


static std::string  sanitizeFilename(std::string name)
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


void POST::handle(const HTTPRequest &request, HttpResponse &response, const LocationBlock &location)
{
    const std::string &target = request.getTarget();

    if (target.find("..") != std::string::npos ||
        target.find("%2e%2e") != std::string::npos ||
        target.find("%2E%2E") != std::string::npos)
    {
        response.setStatus(403, "Forbidden");
        return;
    }

    unsigned long limit = location.getMaxBodySize();
    if (request.getBody().size() > limit)
    {
        handleError(413, "Payload Too Large", response);
        return;
    }
    std::string ct = request.getHeader("Content-Type");

    std::string ctLower = ct;
    for (size_t i = 0; i < ctLower.size(); i++)
        ctLower[i] = tolower(ctLower[i]);

    if (ctLower.find("multipart/form-data") == std::string::npos)
    {
        response.setStatus(415, "Unsupported Media Type");
        return;
    }

    if (ct.empty())
    {
        response.setStatus(400, "Bad Request");
        response.setBody("Missing Content-Type");
        return;
    }

    if (ct.find("multipart/form-data") == std::string::npos)
    {
        response.setStatus(415, "Unsupported Media Type");
        response.setBody("POST supports only multipart/form-data");
        response.setHeader("Content-Type", "text/html");
        return;
    }
    size_t pos = ct.find("boundary=");
    if (pos == std::string::npos)
    {
        response.setStatus(400, "Bad Request");
        response.setBody("Missing multipart boundary");
        return;
    }

    const std::string boundary = "--" + ct.substr(pos + 9);
    if (boundary.empty())
    {
        response.setStatus(400, "Bad Request");
        response.setBody("Empty boundary");
        return;
    }
    try {
        MultipartParser parser(boundary, request.getBody());

        while (parser.hasNextPart())
        {
            Part part = parser.nextPart();
            if (part.filename.empty())
            {
                response.setStatus(400, "Bad Request");
                return;
            }

            try
            {
                std::string filename = sanitizeFilename(part.filename);
                
                if (filename.empty())
                {
                    response.setStatus(400, "Bad Request");
                    return;
                }
                std::string root = location.getRoot();
                if (!root.empty() && root[root.size() - 1] == '/')
                    root.erase(root.size() - 1);

                std::string target = request.getTarget();
                if (target.empty() || target[0] != '/')
                {
                    response.setStatus(400, "Bad Request");
                    return;
                }
                if (target.find("..") != std::string::npos)
                {
                    response.setStatus(400, "Bad Request");
                    return;
                }


                std::string basePath =  root + request.getTarget();
                if (!basePath.empty() && basePath[basePath.size() - 1] != '/')
                  basePath += '/';
                std::string path = basePath + filename;

                struct stat st;
                if (stat(basePath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
                {
                    response.setStatus(403, "Not Found");
                    return;
                }

                if (access(basePath.c_str(), W_OK) != 0)
                {
                    handleError(404, "Forbidden: Upload directory is not writable", response);
                    return;
                }
                std::cout << "Path = " << path << std::endl;
                int i = 1;
              std::string base = filename;
              while (access(path.c_str(), F_OK) == 0)
              {
                std::ostringstream oss;
                oss << i++;
                filename = oss.str() + "_" + base;
                path = basePath + filename;
              }


                std::ofstream file(path.c_str(), std::ios::binary);
                if (!file.is_open())
                {
                    handleError(500, "Internal Server Error", response);
                    return;
                }
                file.write(part.data.data(), part.data.size());
                file.close();
            }
            catch (...)
            {
                if (errno == EFBIG)
                    response.setStatus(413, "Payload Too Large");
                else
                    response.setStatus(400, "Bad Request");
                return;
            }

        }

        response.setStatus(201, "Created");
        response.setBody("Upload successful");
      }
      catch (...) {
          response.setStatus(400, "Bad Request");
          response.setBody("Invalid multipart body");
          return;
      }
      return;
}


#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

enum EventType
{
	READ = 0x01
};

class Logger
{
public:
	enum Level
	{
		INFO,
		ERROR
	};

	static void log(Level level, const std::string &message, const std::string &context)
	{
		const char *prefix = level == INFO ? "[INFO] " : "[ERROR] ";
		std::cerr << prefix << context << ": " << message << std::endl;
	}
};

class HttpResponse
{
private:
	std::string version;
	std::string statusCode;
	std::string statusMessage;
	std::string body;
	std::map<std::string, std::string> headers;

public:
	HttpResponse()
		: version("HTTP/1.1"), statusCode("200"), statusMessage("OK")
	{
	}

	void setVersion(const std::string &v) { version = v; }
	void setStatusCode(const std::string &code) { statusCode = code; }
	void setStatusMessage(const std::string &message) { statusMessage = message; }
	void setBody(const std::string &b) { body = b; }
	const std::string &getBody() const { return body; }

	void setHeader(const std::string &key, const std::string &value)
	{
		headers[key] = value;
	}

	std::string buildResponse() const
	{
		std::ostringstream oss;
		oss << version << " " << statusCode << " " << statusMessage << "\r\n";
		for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
			oss << it->first << ": " << it->second << "\r\n";
		oss << "\r\n"
			<< body;
		return oss.str();
	}

	void generateStandardErrorResponse(const std::string &code, const std::string &message,
									   const std::string &title, const std::string &description)
	{
		setVersion("HTTP/1.1");
		setStatusCode(code);
		setStatusMessage(message);
		std::ostringstream oss;
		oss << "<html><head><title>" << title << "</title></head><body><h1>" << title << "</h1><p>"
			<< description << "</p></body></html>";
		setBody(oss.str());
		setHeader("Content-Type", "text/html");
		setHeader("Content-Length", std::to_string(getBody().length()));
		setHeader("Connection", "close");
	}
};

class HttpRequest
{
private:
	std::string method;
	std::string uri;
	std::string version;
	std::vector<std::string> queries;
	std::map<std::string, std::string> headers;

	static std::string normalizeHeaderName(const std::string &name)
	{
		std::string normalized = name;
		std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
		return normalized;
	}

public:
	HttpRequest(const std::string &method, const std::string &uri, const std::string &version)
		: method(method), uri(uri), version(version)
	{
	}

	void setQueryString(const std::string &queryString)
	{
		queries.clear();
		if (queryString.empty())
			return;
		std::string normalized = queryString;
		std::size_t question = normalized.find('?');
		if (question != std::string::npos)
			normalized = normalized.substr(question + 1);
		std::stringstream ss(normalized);
		std::string token;
		while (std::getline(ss, token, '&'))
		{
			if (!token.empty())
				queries.push_back(token);
		}
	}

	const std::vector<std::string> &getQueries() const { return queries; }

	std::string getHeader(const std::string &name) const
	{
		std::map<std::string, std::string>::const_iterator it = headers.find(normalizeHeaderName(name));
		if (it == headers.end())
			return "";
		return it->second;
	}

	void setHeader(const std::string &name, const std::string &value)
	{
		headers[normalizeHeaderName(name)] = value;
	}

	const std::string &getMethod() const { return method; }
	const std::string &getUri() const { return uri; }
	const std::string &getVersion() const { return version; }
};

class CgiDirective
{
private:
	std::vector<std::string> extensions;
	bool _isEnabled;

public:
	CgiDirective() : _isEnabled(false) {}

	void addCgiExtension(const std::string &extension)
	{
		if (!extension.empty())
		{
			extensions.push_back(extension);
			_isEnabled = true;
		}
	}

	const std::vector<std::string> &getExtensions() const
	{
		return extensions;
	}

	bool isEnabled() const
	{
		return _isEnabled;
	}
};

struct ServerConfig
{
	std::string root;
	CgiDirective cgiExtension;
};

class EventPoller
{
public:
	void registerEvent(int fd, EventType eventType)
	{
		(void)fd;
		(void)eventType;
	}
};

class CgiHandler
{
private:
	int pid;
	int pipeFd[2];
	int postBodyFd;
	int cgiClientSocket;
	std::string cgiResponseMessage;
	std::chrono::time_point<std::chrono::steady_clock> startTime;
	bool isValid;

public:
	CgiHandler(HttpRequest &request, ServerConfig &config, EventPoller *eventManager, int clientSocket, const std::string &postPath = "")
		: pid(-1), postBodyFd(-1), cgiClientSocket(clientSocket), startTime(std::chrono::steady_clock::now()), isValid(true)
	{
		pipeFd[0] = -1;
		pipeFd[1] = -1;
		handleCgiDirective(request, config, eventManager, postPath);
	}

	~CgiHandler()
	{
		if (pipeFd[0] != -1)
		{
			close(pipeFd[0]);
			pipeFd[0] = -1;
		}
		if (postBodyFd != -1)
		{
			close(postBodyFd);
			postBodyFd = -1;
		}
	}

	void addCgiResponseMessage(const std::string &cgiOutput)
	{
		cgiResponseMessage += cgiOutput;
	}

	std::string buildCgiResponse()
	{
		if (cgiResponseMessage.empty())
		{
			HttpResponse response;
			response.generateStandardErrorResponse("502", "Bad Gateway", "Bad Gateway",
												   "The server encountered an unexpected condition which prevented it from fulfilling the request.");
			return response.buildResponse();
		}
		HttpResponse response;
		response.setVersion("HTTP/1.1");
		response.setStatusCode("200");
		response.setStatusMessage("OK");
		response.setBody(cgiResponseMessage);
		response.setHeader("Content-Length", std::to_string(response.getBody().length()));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Server", "Standalone CGI");
		response.setHeader("Connection", "close");
		return response.buildResponse();
	}

	char **initiateEnvVariables(HttpRequest &request, ServerConfig &config)
	{
		std::string fullQuery;
		std::vector<std::string> envVector;

		if (!request.getQueries().empty())
			envVector.insert(envVector.end(), request.getQueries().begin(), request.getQueries().end());
		envVector.push_back("CONTENT_TYPE=" + request.getHeader("content-type"));
		envVector.push_back("CONTENT_LENGTH=" + request.getHeader("content-length"));
		envVector.push_back("HTTP_COOKIE=" + request.getHeader("cookie"));
		for (std::size_t i = 0; i < request.getQueries().size(); ++i)
		{
			if (i != 0)
				fullQuery += "&" + request.getQueries()[i];
			else
				fullQuery += request.getQueries()[i];
		}
		envVector.push_back("QUERY_STRING=" + fullQuery);
		envVector.push_back("HTTP_USER_AGENT=" + request.getHeader("user-agent"));
		envVector.push_back("REQUEST_METHOD=" + request.getMethod());
		envVector.push_back("SERVER_NAME=StandaloneCGI");
		envVector.push_back("SERVER_PROTOCOL=" + request.getVersion());
		envVector.push_back("SCRIPT_FILENAME=" + config.root + request.getUri());
		envVector.push_back("SERVER_NAME=" + request.getHeader("host"));
		envVector.push_back("PATH=" + std::string(getenv("PATH") ? getenv("PATH") : "/usr/bin:/bin"));

		char **envArray = new char *[envVector.size() + 1];
		std::size_t counter = 0;
		for (; counter < envVector.size(); ++counter)
		{
			envArray[counter] = new char[envVector[counter].length() + 1];
			std::strcpy(envArray[counter], envVector[counter].c_str());
		}
		envArray[counter] = NULL;
		return envArray;
	}

	void handleCgiDirective(HttpRequest &request, ServerConfig &config, EventPoller *eventManager, const std::string &postPath)
	{
		char **parameters = NULL;
		char **envp = NULL;

		if (pipe(pipeFd) < 0)
		{
			Logger::log(Logger::ERROR, "Failed to create pipe", "CgiHandler::handleCgiDirective");
			isValid = false;
			return;
		}

		envp = initiateEnvVariables(request, config);
		parameters = new char *[2];
		parameters[0] = new char[config.root.length() + request.getUri().length() + 1];
		std::strcpy(parameters[0], (config.root + request.getUri()).c_str());
		parameters[1] = NULL;

		if (request.getMethod() == "POST")
		{
			postBodyFd = open(postPath.c_str(), O_RDONLY);
			if (postBodyFd < 0)
			{
				Logger::log(Logger::ERROR, "Failed to open POST body file", "CgiHandler::handleCgiDirective");
				isValid = false;
				delete2dArray(parameters);
				delete2dArray(envp);
				return;
			}
		}

		pid = fork();
		if (pid < 0)
		{
			Logger::log(Logger::ERROR, "Failed to fork", "CgiHandler::handleCgiDirective");
			isValid = false;
			delete2dArray(parameters);
			delete2dArray(envp);
			return;
		}
		else if (pid == 0)
		{
			close(pipeFd[0]);
			dup2(pipeFd[1], STDOUT_FILENO);
			close(pipeFd[1]);

			if (request.getMethod() == "POST")
			{
				dup2(postBodyFd, STDIN_FILENO);
				close(postBodyFd);
			}
			if (execve(parameters[0], parameters, envp) < 0)
			{
				delete2dArray(parameters);
				delete2dArray(envp);
				std::exit(EXIT_FAILURE);
			}
		}
		else
		{
			close(pipeFd[1]);
			int flags = fcntl(pipeFd[0], F_GETFL, 0);
			if (flags < 0 || fcntl(pipeFd[0], F_SETFL, flags | O_NONBLOCK) < 0)
			{
				Logger::log(Logger::ERROR, "Failed to configure pipe fd", "CgiHandler::handleCgiDirective");
				isValid = false;
				delete2dArray(parameters);
				delete2dArray(envp);
				return;
			}
			if (eventManager)
				eventManager->registerEvent(pipeFd[0], READ);
			delete2dArray(parameters);
			delete2dArray(envp);
		}
	}

	void delete2dArray(char **str)
	{
		if (!str)
			return;
		for (std::size_t i = 0; str[i]; ++i)
			delete[] str[i];
		delete[] str;
	}

	int getChildPid() const { return pid; }
	int getCgiClientSocket() const { return cgiClientSocket; }
	int getCgiReadFd() const { return pipeFd[0]; }
	const std::string &getCgiResponseMessage() const { return cgiResponseMessage; }

	bool isValidCgi() const { return isValid; }

	bool isTimedOut(std::size_t timeout) const
	{
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		return std::chrono::duration_cast<std::chrono::seconds>(now - startTime) > std::chrono::seconds(timeout);
	}

	static bool fileExists(const std::string &path)
	{
		struct stat fileStat;
		return stat(path.c_str(), &fileStat) == 0;
	}

	static bool validateFileExtension(HttpRequest &request, ServerConfig &config)
	{
		std::vector<std::string> cgiExten = config.cgiExtension.getExtensions();
		std::string uri = request.getUri();
		if (uri.find('.') == std::string::npos)
			return false;
		std::string extension = uri.substr(uri.find('.'));
		return std::find(cgiExten.begin(), cgiExten.end(), extension) != cgiExten.end();
	}

	static bool validCgiRequest(HttpRequest &request, ServerConfig &config)
	{
		return fileExists(config.root + request.getUri()) && validateFileExtension(request, config);
	}
};

static void printUsage(const char *programName)
{
	std::cout << "Usage: " << programName << " <path-to-cgi-script> [query_string] [method] [body]\n";
	std::cout << "Examples:\n";
	std::cout << "  " << programName << " ./cgi-bin/hello.py\n";
	std::cout << "  " << programName << " ./cgi-bin/adder.py \"a=1&b=2\" GET\n";
	std::cout << "  " << programName << " ./cgi-bin/echo.py \"\" POST \"message=hello\"\n";
}

static std::string createTempFileWithContent(const std::string &content)
{
	if (content.empty())
		return "";
	std::string pattern = "/tmp/cgi-body-XXXXXX";
	std::vector<char> buffer(pattern.begin(), pattern.end());
	buffer.push_back('\0');
	int fd = mkstemp(&buffer[0]);
	if (fd == -1)
		return "";
	ssize_t written = write(fd, content.c_str(), content.size());
	(void)written;
	close(fd);
	return std::string(&buffer[0]);
}

static bool ensureExecutable(const std::string &path)
{
	struct stat fileStat;
	if (stat(path.c_str(), &fileStat) != 0)
	{
		Logger::log(Logger::ERROR, std::string("stat failed for ") + path + ": " + std::strerror(errno), "ensureExecutable");
		return false;
	}
	if (!S_ISREG(fileStat.st_mode))
	{
		Logger::log(Logger::ERROR, "CGI target is not a regular file: " + path, "ensureExecutable");
		return false;
	}
	if (fileStat.st_mode & S_IXUSR)
		return true;

	mode_t newMode = fileStat.st_mode | S_IXUSR;
	if (chmod(path.c_str(), newMode) != 0)
	{
		Logger::log(Logger::ERROR, std::string("chmod failed for ") + path + ": " + std::strerror(errno), "ensureExecutable");
		return false;
	}
	return true;
}

<<<<<<< HEAD
int main(int argc, char **argv)
{
	if (argc < 2)
	{
		printUsage(argv[0]);
		return 1;
	}

	std::string scriptPath = argv[1];
	std::string query = argc >= 3 ? argv[2] : "";
	std::string method = argc >= 4 ? argv[3] : "GET";
	std::string body = argc >= 5 ? argv[4] : "";

	std::transform(method.begin(), method.end(), method.begin(), ::toupper);
	if (method != "GET" && method != "POST")
	{
		Logger::log(Logger::ERROR, "Method must be GET or POST", "main");
		return 1;
	}

	std::string rootDir;
	std::string uri;
	std::size_t lastSlash = scriptPath.find_last_of('/');
	if (lastSlash == std::string::npos)
	{
		rootDir = ".";
		uri = "/" + scriptPath;
	}
	else
	{
		rootDir = scriptPath.substr(0, lastSlash);
		if (rootDir.empty())
			rootDir = "/";
		uri = scriptPath.substr(lastSlash);
		if (uri.empty())
			uri = "/cgi-script";
	}

	ServerConfig config;
	config.root = rootDir;
	const std::string knownExtensions[] = {".cgi", ".pl", ".py", ".sh", ".php"};
	for (std::size_t i = 0; i < sizeof(knownExtensions) / sizeof(knownExtensions[0]); ++i)
		config.cgiExtension.addCgiExtension(knownExtensions[i]);
	std::size_t lastDot = scriptPath.find_last_of('.');
	if (lastDot != std::string::npos)
		config.cgiExtension.addCgiExtension(scriptPath.substr(lastDot));

	HttpRequest request(method, uri, "HTTP/1.1");
	request.setQueryString(query);
	request.setHeader("host", "localhost");
	request.setHeader("user-agent", "cgi-standalone-demo/1.0");
	request.setHeader("cookie", "demo=true");
	request.setHeader("content-type", method == "POST" ? "application/x-www-form-urlencoded" : "text/plain");
	request.setHeader("content-length", method == "POST" ? std::to_string(body.length()) : "0");

	if (!CgiHandler::validCgiRequest(request, config))
	{
		Logger::log(Logger::ERROR, "CGI script not found or extension not allowed", "main");
		return 1;
	}

	std::string scriptFsPath = config.root + request.getUri();
	if (!ensureExecutable(scriptFsPath))
	{
		Logger::log(Logger::ERROR, "CGI script is not executable and permissions could not be adjusted", "main");
		return 1;
	}

	std::string postBodyPath;
	if (method == "POST")
	{
		postBodyPath = createTempFileWithContent(body);
		if (postBodyPath.empty())
		{
			Logger::log(Logger::ERROR, "Failed to create temporary POST body file", "main");
			return 1;
		}
	}

	EventPoller poller;
	CgiHandler handler(request, config, &poller, STDOUT_FILENO, postBodyPath);
	if (!handler.isValidCgi())
	{
		Logger::log(Logger::ERROR, "Failed to initialize CGI handler", "main");
		if (!postBodyPath.empty())
			unlink(postBodyPath.c_str());
		return 1;
	}

	int cgiFd = handler.getCgiReadFd();
	if (cgiFd == -1)
	{
		Logger::log(Logger::ERROR, "No CGI output pipe available", "main");
		if (!postBodyPath.empty())
			unlink(postBodyPath.c_str());
		return 1;
	}

	int flags = fcntl(cgiFd, F_GETFL, 0);
	if (flags != -1)
		fcntl(cgiFd, F_SETFL, flags & ~O_NONBLOCK);

	char buffer[4096];
	while (true)
	{
		ssize_t bytesRead = read(cgiFd, buffer, sizeof(buffer));
		if (bytesRead > 0)
		{
			handler.addCgiResponseMessage(std::string(buffer, bytesRead));
		}
		else if (bytesRead == 0)
		{
			break;
		}
		else
		{
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
				continue;
			}
			Logger::log(Logger::ERROR, std::string("Read error: ") + std::strerror(errno), "main");
			break;
		}
	}

	if (handler.getChildPid() > 0)
		waitpid(handler.getChildPid(), NULL, 0);

	std::cout << handler.buildCgiResponse() << std::endl;

	if (!postBodyPath.empty())
		unlink(postBodyPath.c_str());

	return 0;
=======
int main(int argc, char *argv[]) {
  std::string config_file;
  init_conf_file(argc, argv, config_file);
  try {
    std::string config_content = read_config_file(config_file);

    std::vector<std::string> tokens = tokenize_config_file(config_content);
    // For demonstration, print the tokens
    for (size_t i = 0; i < tokens.size(); ++i) {
      std::cout << "Token " << i << ": " << tokens[i] << std::endl;
    }

  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
>>>>>>> parent of dde42b6 (Working on the syntax validator for our conf file)
}

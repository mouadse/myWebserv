# ---------- toolchain ----------
CXX := c++

# ---------- flags ----------
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./src
DEPFLAGS = -MMD -MP

# ---------- sources ----------
SRCS = src/main.cpp \
       src/cgi/CgiHandler.cpp \
       src/config/ConfigurationFile.cpp \
       src/config/LocationBlock.cpp \
       src/config/ParserUtils.cpp \
       src/config/ServerConfigParser.cpp \
       src/config/WebserverConfig.cpp \
       src/http/HTTPRequest.cpp \
       src/http/HTTPResponse.cpp \
       src/http/RequestHandler.cpp \
       src/http/Methods/Delete.cpp \
       src/http/Methods/Get.cpp \
       src/http/Methods/Post.cpp \
       src/server/client.cpp \
       src/server/EpollManager.cpp \
       src/server/server.cpp \
       src/utils/Helpers.cpp \
       src/utils/SignalHandling.cpp \
       src/utils/FileCache.cpp \
       src/utils/Logger.cpp \
       src/utils/PathUtils.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(OBJS:.o=.d)

NAME = webserv

# ---------- rules ----------
all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

.SECONDARY: $(OBJS)

-include $(DEPS)

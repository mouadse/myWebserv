# ---------- toolchain ----------
CCACHE := $(shell command -v ccache 2>/dev/null)
ifeq ($(CCACHE),)
CXX := c++
else
CXX := ccache c++
endif

# Parallel by default (override: make -j1 ...)
MAKEFLAGS += -j$(shell nproc)

# ---------- flags ----------
BASE_CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./src
DEPFLAGS      = -MMD -MP

DEBUG ?= 0
ifeq ($(DEBUG),1)
	CXXFLAGS = $(BASE_CXXFLAGS) -g -O0 -fsanitize=address
else
	CXXFLAGS = $(BASE_CXXFLAGS) -O2
endif

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
       src/http/Methods/MultipartParser.cpp \
       src/server/client.cpp \
       src/server/EpollManager.cpp \
       src/server/server.cpp \
       src/utils/Helpers.cpp \
			 src/utils/SignalHandling.cpp \
       src/utils/FileCache.cpp \
       src/utils/Logger.cpp

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

# IMPORTANT: make re serial even under -j
re:
	@$(MAKE) fclean
	@$(MAKE) all

.PHONY: all clean fclean re

-include $(DEPS)

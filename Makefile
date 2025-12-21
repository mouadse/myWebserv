# ---------- toolchain ----------
UNAME_S := $(shell uname -s)
CCACHE := $(shell command -v ccache 2>/dev/null)
ifeq ($(CCACHE),)
CXX := c++
else
CXX := ccache c++
endif

# Parallel by default (override: make -j1 ...)
MAKEFLAGS += -j$(shell nproc)

# ---------- flags ----------
CXXFLAGS_COMMON = -Wall -Wextra -Werror -std=c++98 -I./src
DEPFLAGS      = -MMD -MP

ifeq ($(UNAME_S),Linux)
	CXXFLAGS_DEV = $(CXXFLAGS_COMMON) -g3 -Wpedantic -Wcast-align -Wcast-qual -Wunused \
		-Woverloaded-virtual -Wmisleading-indentation -Wnon-virtual-dtor\
		-fstack-protector-strong -fstrict-overflow
endif

# Production Flags
CXXFLAGS_PROD = $(CXXFLAGS_COMMON) -O3 -march=native -flto -fstack-protector-strong -D_FORTIFY_SOURCE=2

DEBUG ?= 0
ifeq ($(DEBUG),1)
	CXXFLAGS = $(CXXFLAGS_DEV) -fsanitize=address
else
	CXXFLAGS = $(CXXFLAGS_PROD)
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

# IMPORTANT: make re serial even under -j
re:
	@$(MAKE) fclean
	@$(MAKE) all

.PHONY: all clean fclean re

-include $(DEPS)

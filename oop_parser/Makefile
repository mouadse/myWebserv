CXX := g++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -Iinclude -g3

SRC := src/main.cpp \
	src/ConfigFile.cpp \
	src/ParserUtils.cpp \
	src/Location.cpp \
	src/ServerConfig.cpp \
	src/ConfigParser.cpp

OBJ := $(SRC:.cpp=.o)
TARGET := config_parser

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(TARGET)

re: fclean all

.PHONY: all clean fclean re

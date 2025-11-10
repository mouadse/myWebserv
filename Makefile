NAME := webserv
SRCS := main.cpp
OBJS := $(SRCS:.cpp=.o)

TEST_DIR := tests
TEST_SRCS := $(TEST_DIR)/test_config_validation.cpp
TEST_BIN := $(TEST_DIR)/config_validation_tests

CXX := clang++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -pedantic
RM := rm -f

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_BIN): $(TEST_SRCS) main.cpp
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $(TEST_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	$(RM) $(OBJS) $(TEST_BIN)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re test

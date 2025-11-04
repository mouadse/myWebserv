NAME := webserv
SRCS := main.cpp
OBJS := $(SRCS:.cpp=.o)

CXX := clang++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -pedantic
RM := rm -f

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
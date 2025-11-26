CXX := g++
TARGET := config_parser
TEST_TARGET := parser_tests

CORE_SRC := ConfigurationFile.cpp \
	ParserUtils.cpp \
	LocationBlock.cpp \
	WebserverConfig.cpp \
	ServerConfigParser.cpp
MAIN_SRC := main.cpp
SRC := $(MAIN_SRC) $(CORE_SRC)

BUILD_DIR := build
OBJ := $(SRC:%.cpp=$(BUILD_DIR)/%.o)
CORE_OBJ := $(CORE_SRC:%.cpp=$(BUILD_DIR)/%.o)
TEST_SRC := tests/test_runner.cpp
TEST_OBJ := $(TEST_SRC:%.cpp=$(BUILD_DIR)/%.o)

CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -I.

DEBUG_FLAGS := -Og -g3 -ggdb3 -fno-omit-frame-pointer -fno-inline
RELEASE_FLAGS := -O3 -DNDEBUG

MODE ?= debug
ifeq ($(MODE),release)
	CXXFLAGS += $(RELEASE_FLAGS)
else
	CXXFLAGS += $(DEBUG_FLAGS)
endif

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	@echo "🔧 Linking $(TARGET) [$(MODE)]..."
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

$(TEST_TARGET): $(CORE_OBJ) $(TEST_OBJ)
	@mkdir -p $(BUILD_DIR)
	@echo "🧪 Linking $(TEST_TARGET) [$(MODE)]..."
	$(CXX) $(CXXFLAGS) -o $@ $(CORE_OBJ) $(TEST_OBJ)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "🧩 Compiling $< -> $@"
	$(CXX) $(CXXFLAGS) -c $< -o $@

TEST_FILTER ?=
test: $(TEST_TARGET)
	@echo "🧪 Running parser tests..."
	@./$(TEST_TARGET) $(TEST_FILTER)

clean:
	@echo "🧹 Cleaning object files..."
	rm -rf $(BUILD_DIR)

fclean: clean
	@echo "🗑️  Removing binary..."
	rm -f $(TARGET) $(TEST_TARGET)

re: fclean all

.PHONY: all clean fclean re test

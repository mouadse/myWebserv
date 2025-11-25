CXX := g++
TARGET := config_parser
TEST_TARGET := parser_tests

CORE_SRC := src/ConfigFile.cpp \
	src/ParserUtils.cpp \
	src/Location.cpp \
	src/ServerConfig.cpp \
	src/ConfigParser.cpp
MAIN_SRC := src/main.cpp
SRC := $(MAIN_SRC) $(CORE_SRC)
# === Build directories ===
BUILD_DIR := build
OBJ := $(SRC:src/%.cpp=$(BUILD_DIR)/%.o)
CORE_OBJ := $(CORE_SRC:src/%.cpp=$(BUILD_DIR)/%.o)
TEST_SRC := tests/test_runner.cpp
TEST_OBJ := $(BUILD_DIR)/tests/test_runner.o

# === Common flags ===
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -Iinclude

# === Build modes ===
DEBUG_FLAGS := -Og -g3 -ggdb3 -fno-omit-frame-pointer -fno-inline
RELEASE_FLAGS := -O3 -DNDEBUG

# Use "make MODE=release" for release builds; default = debug
MODE ?= debug
ifeq ($(MODE),release)
	CXXFLAGS += $(RELEASE_FLAGS)
else
	CXXFLAGS += $(DEBUG_FLAGS)
endif

# === Targets ===
all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "🔧 Linking $(TARGET) [$(MODE)]..."
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

$(TEST_TARGET): $(CORE_OBJ) $(TEST_OBJ)
	@echo "🧪 Linking $(TEST_TARGET) [$(MODE)]..."
	$(CXX) $(CXXFLAGS) -o $@ $(CORE_OBJ) $(TEST_OBJ)

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo "🧩 Compiling $< -> $@"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	@echo "🧪 Compiling $< -> $@"
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

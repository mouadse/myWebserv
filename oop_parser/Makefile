CXX := g++
TARGET := config_parser

SRC := src/main.cpp \
	src/ConfigFile.cpp \
	src/ParserUtils.cpp \
	src/Location.cpp \
	src/ServerConfig.cpp \
	src/ConfigParser.cpp

# === Build directories ===
BUILD_DIR := build
OBJ := $(SRC:src/%.cpp=$(BUILD_DIR)/%.o)

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

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo "🧩 Compiling $< -> $@"
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "🧹 Cleaning object files..."
	rm -rf $(BUILD_DIR)

fclean: clean
	@echo "🗑️  Removing binary..."
	rm -f $(TARGET)

re: fclean all

.PHONY: all clean fclean re

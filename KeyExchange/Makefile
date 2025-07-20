# ====================================================
#   Copyright (C) 2024 CloudCoinConsortium
#
#   Author        : RKE Implementation Team
#   File Name     : Makefile
#   Last Modified : 2024-07-20
#   Describe      : Build system for RKE implementation
#
# ====================================================

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
CFLAGS_DEBUG = -Wall -Wextra -std=c99 -g -DDEBUG
CFLAGS_STRICT = -Wall -Werror -Wextra -std=c99 -Wno-error=format-truncation

# Directories
SRC_DIR = src
RKE_DIR = $(SRC_DIR)/rke
COMMON_DIR = $(SRC_DIR)/common
TEST_DIR = tests
BUILD_DIR = build

# Include paths
INCLUDES = -I$(SRC_DIR) -I$(COMMON_DIR) -I$(RKE_DIR)

# Source files
RKE_SOURCES = $(RKE_DIR)/rke_core.c \
              $(RKE_DIR)/rke_storage.c \
              $(RKE_DIR)/rke_crypto.c \
              $(RKE_DIR)/rke_protocol.c

COMMON_SOURCES = # Common sources are header-only for now

ALL_SOURCES = $(RKE_SOURCES) $(COMMON_SOURCES)

# Object files
RKE_OBJECTS = $(RKE_SOURCES:$(RKE_DIR)/%.c=$(BUILD_DIR)/rke/%.o)
COMMON_OBJECTS = $(COMMON_SOURCES:$(COMMON_DIR)/%.c=$(BUILD_DIR)/common/%.o)
ALL_OBJECTS = $(RKE_OBJECTS) $(COMMON_OBJECTS)

# Test files
TEST_SOURCES = $(TEST_DIR)/test_rke_core.c $(TEST_DIR)/test_rke_protocol.c
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(BUILD_DIR)/tests/%.o)
TEST_EXECUTABLES = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(BUILD_DIR)/%)

# Library
LIBRKE = $(BUILD_DIR)/librke.a

# Default target
all: $(LIBRKE) tests

# Create directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/rke
	@mkdir -p $(BUILD_DIR)/common
	@mkdir -p $(BUILD_DIR)/tests

# Compile RKE source files
$(BUILD_DIR)/rke/%.o: $(RKE_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile common source files
$(BUILD_DIR)/common/%.o: $(COMMON_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Create RKE library
$(LIBRKE): $(ALL_OBJECTS)
	@echo "Creating library $@"
	ar rcs $@ $^
	@echo "Library created successfully"

# Compile test files
$(BUILD_DIR)/tests/%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling test $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build test executables
$(BUILD_DIR)/test_%: $(BUILD_DIR)/tests/test_%.o $(LIBRKE)
	@echo "Building test executable $@"
	$(CC) $< -L$(BUILD_DIR) -lrke -o $@

# Build all tests
tests: $(TEST_EXECUTABLES)
	@echo "All tests built successfully"

# Run tests
test: tests
	@echo "Running tests..."
	@for test in $(TEST_EXECUTABLES); do \
		echo "Running $$test"; \
		./$$test || exit 1; \
	done
	@echo "All tests passed!"

# Debug build
debug: CFLAGS = $(CFLAGS_DEBUG)
debug: clean all
	@echo "Debug build completed"

# Strict compilation (warnings as errors)
strict: CFLAGS = $(CFLAGS_STRICT)
strict: clean all
	@echo "Strict compilation completed"

# Install library (if needed)
install: $(LIBRKE)
	@echo "Installing RKE library..."
	@mkdir -p /usr/local/lib
	@mkdir -p /usr/local/include/rke
	cp $(LIBRKE) /usr/local/lib/
	cp $(RKE_DIR)/rke.h /usr/local/include/rke/
	@echo "Installation completed"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo "Clean completed"

# Force rebuild
rebuild: clean all

# Check syntax without linking
check:
	@echo "Checking syntax..."
	$(CC) $(CFLAGS_STRICT) $(INCLUDES) -fsyntax-only $(ALL_SOURCES)
	@echo "Syntax check passed"

# Show help
help:
	@echo "RKE Build System"
	@echo "================"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build library and tests (default)"
	@echo "  tests     - Build test executables"
	@echo "  test      - Run all tests"
	@echo "  debug     - Build with debug symbols"
	@echo "  strict    - Build with warnings as errors"
	@echo "  check     - Syntax check only"
	@echo "  clean     - Remove build artifacts"
	@echo "  rebuild   - Clean and build"
	@echo "  install   - Install library system-wide"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make                 # Build everything"
	@echo "  make strict          # Strict compilation"
	@echo "  make test            # Run tests"
	@echo "  make clean rebuild   # Clean rebuild"

# Show build information
info:
	@echo "Build Information"
	@echo "=================="
	@echo "Compiler: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Sources: $(words $(ALL_SOURCES)) files"
	@echo "Tests: $(words $(TEST_SOURCES)) files"
	@echo "Build Directory: $(BUILD_DIR)"

# Phony targets
.PHONY: all tests test debug strict install clean rebuild check help info

# Make build directory creation order-only prerequisite
$(ALL_OBJECTS): | $(BUILD_DIR)
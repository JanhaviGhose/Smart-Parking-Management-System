# Makefile for Smart Parking Management System
#
#   make all    - build the application
#   make run    - build and run the application
#   make test   - build and run the unit tests (Google Test)
#   make clean  - remove all generated files

CC        := gcc
CXX       := g++
CFLAGS    := -std=c11 -Wall -Wextra -Wpedantic -Iinclude -MMD -MP
CXXFLAGS  := -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -MMD -MP
GTEST_LIBS ?= -lgtest -lgtest_main -pthread

SRC_DIR   := src
TEST_DIR  := tests
BUILD_DIR := build
DATA_DIR  := data

APP       := $(BUILD_DIR)/smart_parking
TEST_APP  := $(BUILD_DIR)/run_tests

# All .c files; main.c is left out of the test build because gTest has its own main().
SRCS      := $(wildcard $(SRC_DIR)/*.c)
LIB_SRCS  := $(filter-out $(SRC_DIR)/main.c,$(SRCS))
TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)

OBJS      := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
LIB_OBJS  := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(LIB_SRCS))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_SRCS))
DEPS      := $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)

.PHONY: all run test clean

all: $(APP)

# Link the application
$(APP): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Compile each C file separately into an object file
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile each test file
$(BUILD_DIR)/tests/%.o: $(TEST_DIR)/%.cpp | $(BUILD_DIR)/tests
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link the test program
$(TEST_APP): $(LIB_OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(GTEST_LIBS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/tests:
	mkdir -p $(BUILD_DIR)/tests

run: $(APP)
	mkdir -p $(DATA_DIR)
	./$(APP)

test: $(TEST_APP)
	./$(TEST_APP)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TEST_DIR)/tmp_*.txt $(TEST_DIR)/tmp_*.txt.tmp

# Rebuild only what changed when a header changes
-include $(DEPS)

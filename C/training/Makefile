
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -pthread -I./include
LDFLAGS = -pthread -lm

# Directories
SRC_TABLES = tables
SRC_CURRICULUM = curriculum
SRC_TESTS = tests
BUILD_DIR = build
DATA_DIR = ../data/training

# Source files
TABLE_SRCS = $(SRC_TABLES)/table_common.c \
             $(SRC_TABLES)/math_tables.c \
             $(SRC_TABLES)/logic_tables.c \
             $(SRC_TABLES)/language_tables.c \
             $(SRC_TABLES)/code_tables.c

CURRICULUM_SRCS = $(SRC_CURRICULUM)/curriculum_controller.c \
                  $(SRC_CURRICULUM)/progress_tracker.c \
                  $(SRC_CURRICULUM)/accuracy_gates.c

# Object files
TABLE_OBJS = $(TABLE_SRCS:%.c=$(BUILD_DIR)/%.o)
CURRICULUM_OBJS = $(CURRICULUM_SRCS:%.c=$(BUILD_DIR)/%.o)

# Targets
GENERATE_TABLES = generate_tables
TEST_TABLES = test_tables
TEST_CURRICULUM = test_curriculum

.PHONY: all clean test generate validate

all: $(GENERATE_TABLES) $(TEST_TABLES) $(TEST_CURRICULUM)

# Create build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/$(SRC_TABLES)
	mkdir -p $(BUILD_DIR)/$(SRC_CURRICULUM)
	mkdir -p $(BUILD_DIR)/$(SRC_TESTS)

# Build table objects
$(BUILD_DIR)/$(SRC_TABLES)/%.o: $(SRC_TABLES)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build curriculum objects
$(BUILD_DIR)/$(SRC_CURRICULUM)/%.o: $(SRC_CURRICULUM)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build test objects
$(BUILD_DIR)/$(SRC_TESTS)/%.o: $(SRC_TESTS)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Generate tables executable
$(GENERATE_TABLES): $(TABLE_OBJS) generate_tables.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) generate_tables.c $(TABLE_OBJS) -o $@ $(LDFLAGS)

# Test tables executable
$(TEST_TABLES): $(TABLE_OBJS) $(BUILD_DIR)/$(SRC_TESTS)/test_tables.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(BUILD_DIR)/$(SRC_TESTS)/test_tables.o $(TABLE_OBJS) -o $@ $(LDFLAGS)

# Test curriculum executable
$(TEST_CURRICULUM): $(TABLE_OBJS) $(CURRICULUM_OBJS) $(BUILD_DIR)/$(SRC_TESTS)/test_curriculum.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(BUILD_DIR)/$(SRC_TESTS)/test_curriculum.o $(TABLE_OBJS) $(CURRICULUM_OBJS) -o $@ $(LDFLAGS)

# Generate training data
generate: $(GENERATE_TABLES)
	mkdir -p $(DATA_DIR)
	./$(GENERATE_TABLES) $(DATA_DIR)

# Validate generated data
validate: $(GENERATE_TABLES)
	./$(GENERATE_TABLES) $(DATA_DIR)

# Run tests
test: $(TEST_TABLES) $(TEST_CURRICULUM)
	@echo "Running table tests..."
	./$(TEST_TABLES)
	@echo ""
	@echo "Running curriculum tests..."
	./$(TEST_CURRICULUM)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(GENERATE_TABLES) $(TEST_TABLES) $(TEST_CURRICULUM)
	rm -f /tmp/test_*.dat
	rm -f /tmp/progress_*.dat
	rm -f /tmp/checkpoint_*.dat

# Clean generated data
clean-data:
	rm -f $(DATA_DIR)/*.dat

.DEFAULT_GOAL := all

# Makefile for BDI Phase 6: AI Trainer

CC = gcc
CFLAGS = -std=gnu2x -Wall -Wextra -O2 -march=native -I..
LDFLAGS = -lm

# Source files
AUTODIFF_SRCS = autodiff/forward_ad.c autodiff/reverse_ad.c autodiff/gradient.c
OPTIMIZER_SRCS = optimizers/sgd.c optimizers/adam.c optimizers/rmsprop.c optimizers/lr_scheduler.c
LOSS_SRCS = loss/loss.c
METRICS_SRCS = metrics/metrics.c
TRAINING_SRCS = training/training.c

ALL_SRCS = $(AUTODIFF_SRCS) $(OPTIMIZER_SRCS) $(LOSS_SRCS) $(METRICS_SRCS) $(TRAINING_SRCS)
ALL_OBJS = $(ALL_SRCS:.c=.o)

# Library
LIBTRAINER = libtrainer.a

# Test executables
TEST_DIR = ../tests/phase6
TESTS = $(TEST_DIR)/test_forward_ad $(TEST_DIR)/test_reverse_ad \
        $(TEST_DIR)/test_gradient $(TEST_DIR)/test_optimizers \
        $(TEST_DIR)/test_loss_metrics $(TEST_DIR)/test_all_trainer

.PHONY: all clean test

all: $(LIBTRAINER)

$(LIBTRAINER): $(ALL_OBJS)
	ar rcs $@ $^
	@echo "Built libtrainer.a"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build tests
test: $(TESTS)
	@echo "Running Phase 6 tests..."
	@for test in $(TESTS); do \
		if [ -f $$test ]; then \
			echo ""; \
			echo "Running $$test..."; \
			$$test || true; \
		fi \
	done

$(TEST_DIR)/test_forward_ad: $(TEST_DIR)/test_forward_ad.c $(LIBTRAINER)
	$(CC) $(CFLAGS) $< -L. -ltrainer $(LDFLAGS) -o $@

$(TEST_DIR)/test_reverse_ad: $(TEST_DIR)/test_reverse_ad.c $(LIBTRAINER)
	$(CC) $(CFLAGS) $< -L. -ltrainer $(LDFLAGS) -o $@

$(TEST_DIR)/test_gradient: $(TEST_DIR)/test_gradient.c $(LIBTRAINER)
	$(CC) $(CFLAGS) $< -L. -ltrainer $(LDFLAGS) -o $@

$(TEST_DIR)/test_optimizers: $(TEST_DIR)/test_optimizers.c $(LIBTRAINER)
	$(CC) $(CFLAGS) $< -L. -ltrainer $(LDFLAGS) -o $@

$(TEST_DIR)/test_loss_metrics: $(TEST_DIR)/test_loss_metrics.c $(LIBTRAINER)
	$(CC) $(CFLAGS) $< -L. -ltrainer $(LDFLAGS) -o $@

$(TEST_DIR)/test_all_trainer: $(TEST_DIR)/test_all_trainer.c $(LIBTRAINER)
	$(CC) $(CFLAGS) $< -L. -ltrainer $(LDFLAGS) -o $@

clean:
	rm -f $(ALL_OBJS) $(LIBTRAINER) $(TESTS)
	@echo "Cleaned Phase 6 build artifacts"

.PHONY: help
help:
	@echo "BDI Phase 6: AI Trainer Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build libtrainer.a"
	@echo "  test    - Build and run all tests"
	@echo "  clean   - Remove build artifacts"
	@echo ""
	@echo "Components:"
	@echo "  - Automatic Differentiation (forward & reverse mode)"
	@echo "  - Optimizers (SGD, Adam, RMSprop)"
	@echo "  - Learning Rate Schedulers"
	@echo "  - Loss Functions (MSE, CE, BCE, MAE, Huber)"
	@echo "  - Metrics (Accuracy, Precision, Recall, F1)"
	@echo "  - Training Infrastructure"

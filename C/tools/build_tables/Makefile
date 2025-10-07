CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -O2 -g
LDFLAGS = 

SOURCES = make_unidata.c compress.c parse_ucd.c parse_emoji.c parse_collation.c parse_idna.c parse_unihan.c embed_gen.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = make_unidata

DATA_DIR = ../../data/unicode
OUTPUT_DIR = ../../compiler/AIBase/data
EMBED_DIR = ../../compiler/AIBase/embeddings

.PHONY: all clean test generate install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c unicode_types.h
	$(CC) $(CFLAGS) -c $<

generate: $(TARGET)
	@echo "Generating Unicode data files..."
	@mkdir -p $(OUTPUT_DIR) $(EMBED_DIR)
	./$(TARGET)

install: generate
	@echo "Unicode data files generated and ready for use"

clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f $(OUTPUT_DIR)/*.dat
	rm -f $(EMBED_DIR)/unicode_embed.h

test: $(TARGET)
	@echo "Running basic tests..."
	@./$(TARGET)
	@echo "Verifying generated files..."
	@ls -lh $(OUTPUT_DIR)/*.dat
	@echo "Tests completed"

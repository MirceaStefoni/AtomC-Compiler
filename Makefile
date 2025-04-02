CC = gcc
CFLAGS = -Wall -I include
TARGET = p.exe
SRC = src/main.c src/lexer.c src/parser.c src/utils.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean 
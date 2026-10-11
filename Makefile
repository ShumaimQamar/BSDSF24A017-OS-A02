CC = gcc
CFLAGS = -Wall -Wextra -g

SRC = src/lsv1.0.0.c
TARGET = bin/lsv1.0.0

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) obj/*.o

CC = gcc
CFLAGS = -Iinclude -Wall
LDFLAGS = -lpthread

TARGET = lab3
SRCS = src/main.c src/logger.c src/shared_memory.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET) process.log
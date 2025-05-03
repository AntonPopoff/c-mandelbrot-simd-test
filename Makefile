CC = gcc
FLAGS = -MMD -MP -std=c99 -O3 -Wall -Wpedantic -Werror -mavx2 -fopenmp

TARGET = m

SOURCES = $(wildcard *.c)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
DEPS = $(patsubst %.c, %.d, $(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(FLAGS) -o $@ $^ -lraylib -lm

%.o: %.c
	$(CC) $(FLAGS) -c $<

clean:
	rm -rf $(TARGET) $(OBJECTS) $(DEPS)

-include $(DEPS)

.PHONY: all clean


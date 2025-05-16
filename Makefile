CC = gcc
FLAGS = -std=c99 -O3 -Wall -Werror -Wpedantic -MMD -MP -fopenmp -march=native
LFLAGS = -lraylib -lm

TARGET = m
SOURCES = $(wildcard *.c)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
DEPS = $(patsubst %.c, %.d, $(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(FLAGS) -o $@ $^ $(LFLAGS)

-include $(DEPS)

%.o: %.c
	$(CC) $(FLAGS) -c $<

clean:
	rm -rf $(TARGET) $(OBJECTS) $(DEPS)

.PHONY: all clean


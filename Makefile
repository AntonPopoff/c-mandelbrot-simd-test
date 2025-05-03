CC = gcc
FLAGS = -MMD -MP -std=c99 -O3 -Wall -Wpedantic -Werror -mavx2 -fopenmp

TARGET = m

INCLUDE_PATH = "./raylib/include/"
LINK_PATH = "./raylib/lib/"

SOURCES = $(wildcard *.c)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
DEPS = $(patsubst %.c, %.d, $(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(FLAGS) -o $@ $^ -L $(LINK_PATH) -l:libraylib.a -lm

%.o: %.c
	$(CC) $(FLAGS) -I $(INCLUDE_PATH) -c $<

clean:
	rm -rf $(TARGET) $(OBJECTS) $(DEPS)

-include $(DEPS)

.PHONY: all clean


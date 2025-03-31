CC = gcc
FLAGS = -std=c99 -fopenmp -O3 -Wall -Wpedantic -Werror -mavx2 -mavx512f

INCLUDE_PATH = "./raylib/include/"
LINK_PATH = "./raylib/lib/"

TARGET = mandelbrot
SOURCES = $(wildcard *.c)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(FLAGS) -o $@ $^ -L $(LINK_PATH) -l:libraylib.a -lm

%.o: %.c
	$(CC) $(FLAGS) -I $(INCLUDE_PATH) -c $^

clean:
	rm -rf $(TARGET) $(OBJECTS)

.PHONY: clean


# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
LIBS = -lGL -lGLU -lglut -lm
TARGET = simulation
SRC = simulation.c config.c shared.c visualization.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

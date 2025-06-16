# Makefile
CC = gcc
CFLAGS = -Wall -lm

# OpenGL and GLUT flags
GLUT_FLAGS = -lGL -lGLU -lglut

# Shared object files
SHARED_OBJS = shared_memory.o config.o

all: main file_generator calculator inspector_type1 inspector_type2 inspector_type3 visualization

shared_memory.o: shared_memory.c shared_memory.h
	$(CC) $(CFLAGS) -c shared_memory.c

config.o: config.c config.h
	$(CC) $(CFLAGS) -c config.c

main: main.c $(SHARED_OBJS)
	$(CC) $(CFLAGS) -o main main.c shared_memory.o config.o -lrt 

file_generator: file_generator.c $(SHARED_OBJS)
	$(CC) $(CFLAGS) -o file_generator file_generator.c shared_memory.o config.o -lrt 

calculator: calculator.c $(SHARED_OBJS)
	$(CC) $(CFLAGS) -o calculator calculator.c shared_memory.o config.o -lrt 

inspector_type1: inspector_type1.c $(SHARED_OBJS)
	$(CC) $(CFLAGS) -o inspector_type1 inspector_type1.c shared_memory.o config.o -lrt 

inspector_type2: inspector_type2.c $(SHARED_OBJS)
	$(CC) $(CFLAGS) -o inspector_type2 inspector_type2.c shared_memory.o config.o -lrt 

inspector_type3: inspector_type3.c $(SHARED_OBJS)
	$(CC) $(CFLAGS) -o inspector_type3 inspector_type3.c shared_memory.o config.o -lrt 

visualization: visualization.c $(SHARED_OBJS)
	$(CC) $(CFLAGS) -o visualization visualization.c shared_memory.o config.o $(GLUT_FLAGS) -lrt 

clean:
	rm -f *.o main file_generator calculator inspector_type1 inspector_type2 inspector_type3 visualization

// visualization.h
#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include "shared.h"

// Function to initialize OpenGL
int init_opengl(int argc, char **argv);

// Function to render the simulation state
void render(SharedData *shared);

#endif

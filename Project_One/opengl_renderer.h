// opengl_renderer.h
#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include <GL/glut.h>
#include "game_shared.h"

// Visual Representation of a Player
typedef struct {
    float x, y;
    float energy;
    int isJumping;
} VisualPlayer;

// Extern variable for visualTeams
extern VisualPlayer visualTeams[NUM_TEAMS][NUM_PLAYERS];

// Function prototypes
void initOpenGL(int argc, char **argv);
void startOpenGLLoop();
void cleanup_and_exit();

#endif 

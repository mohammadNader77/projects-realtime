// visualization.c
#include "visualization.h"
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h> // Included for cosf and sinf

// Global pointer to shared data for access in callbacks
SharedData *global_shared_data = NULL;

// Window dimensions
int window_width = 1200;
int window_height = 600;

// Define areas
float simulation_width_ratio = 0.7f;
float scoreboard_width_ratio = 0.3f;

// Fonts
void *font = GLUT_BITMAP_HELVETICA_18;

// Function to draw text on the screen
void draw_text(float x, float y, const char *text, float r, float g, float b) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (const char *c = text; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}

// Function to draw a circle representing a resistance group
void draw_circle(float x, float y, float radius, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x, y); // Center of circle
        int num_segments = 20;
        for(int i = 0; i <= num_segments; i++) { 
            float angle = 2.0f * M_PI * i / num_segments;
            float dx = radius * cosf(angle);
            float dy = radius * sinf(angle);
            glVertex2f(x + dx, y + dy);
        }
    glEnd();
}

// Function to draw an agency member as a distinct shape (e.g., diamond)
void draw_agency_member(float x, float y, float size, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_POLYGON);
        glVertex2f(x, y + size);
        glVertex2f(x + size, y);
        glVertex2f(x, y - size);
        glVertex2f(x - size, y);
    glEnd();
}

// Display callback for GLUT
void display_callback() {
    if (global_shared_data == NULL) {
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Define the viewport for simulation graphics (left 70%)
    int sim_width = window_width * simulation_width_ratio;
    glViewport(0, 0, sim_width, window_height);

    // Set up the coordinate system for simulation graphics
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-100.0, 100.0, -100.0, 100.0); // Expanded coordinate system
    glMatrixMode(GL_MODELVIEW);

    // Draw background grid
    glColor3f(0.2f, 0.2f, 0.2f); // Dark gray grid lines
    glLineWidth(0.5f);
    glBegin(GL_LINES);
        for(int i = -100; i <= 100; i += 10) {
            // Vertical lines
            glVertex2f(i, -100.0f);
            glVertex2f(i, 100.0f);
            // Horizontal lines
            glVertex2f(-100.0f, i);
            glVertex2f(100.0f, i);
        }
    glEnd();

    // Draw Resistance Groups
    sem_wait(&global_shared_data->group_semaphore);
    for (int i = 0; i < global_shared_data->resistance_groups_count; i++) {
        ResistanceGroup group = global_shared_data->resistance_groups[i];
        if (group.has_spy) {
            draw_circle(group.x, group.y, 5.0f, 1.0f, 0.0f, 0.0f); // Red for groups with spies
        } else if (group.is_military) {
            draw_circle(group.x, group.y, 5.0f, 0.0f, 0.0f, 1.0f); // Blue for military groups
        } else {
            draw_circle(group.x, group.y, 5.0f, 0.0f, 1.0f, 0.0f); // Green for social groups
        }
    }
    sem_post(&global_shared_data->group_semaphore);

    // Draw Agency Members
    sem_wait(&global_shared_data->semaphore);
    for (int i = 0; i < global_shared_data->current_agency_members && i < MAX_AGENCY_MEMBERS; i++) {
        float x = global_shared_data->agency_positions[i][0];
        float y = global_shared_data->agency_positions[i][1];
        draw_agency_member(x, y, 3.0f, 1.0f, 1.0f, 0.0f); // Yellow diamonds
    }
    sem_post(&global_shared_data->semaphore);

    // Draw Caught Resistance Members
    sem_wait(&global_shared_data->semaphore);
    for (int i = 0; i < global_shared_data->caught_resistance_positions_count && i < MAX_CAUGHT_RESISTANCE; i++) {
        float x = global_shared_data->caught_resistance_positions[i][0];
        float y = global_shared_data->caught_resistance_positions[i][1];
        glColor3f(0.5f, 0.0f, 0.5f); // Purple
        glBegin(GL_QUADS);
            glVertex2f(x - 1.0f, y - 1.0f);
            glVertex2f(x + 1.0f, y - 1.0f);
            glVertex2f(x + 1.0f, y + 1.0f);
            glVertex2f(x - 1.0f, y + 1.0f);
        glEnd();
    }

    // Draw Caught Agency Members
    for (int i = 0; i < global_shared_data->caught_agency_positions_count && i < MAX_CAUGHT_AGENCY; i++) {
        float x = global_shared_data->caught_agency_positions[i][0];
        float y = global_shared_data->caught_agency_positions[i][1];
        glColor3f(1.0f, 0.0f, 1.0f); // Magenta
        glBegin(GL_TRIANGLES);
            glVertex2f(x, y + 1.0f);
            glVertex2f(x - 1.0f, y - 1.0f);
            glVertex2f(x + 1.0f, y - 1.0f);
        glEnd();
    }
    sem_post(&global_shared_data->semaphore);

    // Reset to default viewport for scoreboard (right 30%)
    int sb_x = sim_width;
    glViewport(sb_x, 0, window_width - sim_width, window_height);

    // Set up orthographic projection for 2D text
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, window_width - sim_width, 0, window_height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw Scoreboard Background
    glColor3f(0.1f, 0.1f, 0.1f); // Dark gray background
    glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(window_width - sim_width, 0);
        glVertex2f(window_width - sim_width, window_height);
        glVertex2f(0, window_height);
    glEnd();

    // Define scoreboard sections
    //float section_width = (window_width - sim_width) - 40.0f; // Padding
    float start_x = 20.0f;
    float start_y = window_height - 40.0f;
    float line_height = 30.0f;

    // Draw Title
    draw_text(start_x, start_y, "Scoreboard", 1.0f, 1.0f, 1.0f);

    // Resistance Statistics
    start_y -= line_height;
    draw_text(start_x, start_y, "Resistance Statistics:", 0.0f, 1.0f, 0.0f);
    start_y -= line_height;
    char buffer[256];
    
    sem_wait(&global_shared_data->semaphore);
    sprintf(buffer, "Killed: %d", global_shared_data->killed_resistance);
    draw_text(start_x + 20.0f, start_y, buffer, 1.0f, 0.0f, 0.0f);
    start_y -= line_height;

    sprintf(buffer, "Injured: %d", global_shared_data->injured_resistance);
    draw_text(start_x + 20.0f, start_y, buffer, 1.0f, 0.5f, 0.0f);
    start_y -= line_height;

    sprintf(buffer, "Caught: %d", global_shared_data->caught_resistance);
    draw_text(start_x + 20.0f, start_y, buffer, 1.0f, 0.5f, 0.0f);
    start_y -= (line_height * 1.5f);

    // Agency Statistics
    draw_text(start_x, start_y, "Agency Statistics:", 1.0f, 1.0f, 0.0f);
    start_y -= line_height;
    
    sprintf(buffer, "Killed: %d", global_shared_data->killed_agency);
    draw_text(start_x + 20.0f, start_y, buffer, 1.0f, 0.0f, 1.0f);
    start_y -= line_height;

    sprintf(buffer, "Injured: %d", global_shared_data->injured_agency);
    draw_text(start_x + 20.0f, start_y, buffer, 1.0f, 0.5f, 1.0f);
    start_y -= line_height;

    sprintf(buffer, "Caught: %d", global_shared_data->caught_agency);
    draw_text(start_x + 20.0f, start_y, buffer, 1.0f, 0.5f, 1.0f);
    start_y -= (line_height * 1.5f);

    // Arrests
    draw_text(start_x, start_y, "Arrests:", 0.0f, 0.0f, 1.0f);
    start_y -= line_height;
    
    sprintf(buffer, "Total Arrests: %d", global_shared_data->total_arrests);
    draw_text(start_x + 20.0f, start_y, buffer, 0.0f, 1.0f, 1.0f);
    start_y -= line_height;

    sprintf(buffer, "Total Imprisoned: %d", global_shared_data->total_imprisoned);
    draw_text(start_x + 20.0f, start_y, buffer, 0.0f, 1.0f, 1.0f);
    start_y -= line_height;

    sprintf(buffer, "Total Released: %d", global_shared_data->total_released);
    draw_text(start_x + 20.0f, start_y, buffer, 0.0f, 1.0f, 1.0f);
    sem_post(&global_shared_data->semaphore);

    // Additional Statistics (optional)
    start_y -= (line_height * 1.5f);
    sem_wait(&global_shared_data->semaphore);
    sprintf(buffer, "Total Resistance Groups: %d", global_shared_data->total_resistance_groups);
    draw_text(start_x, start_y, buffer, 1.0f, 1.0f, 1.0f);
    sem_post(&global_shared_data->semaphore);

    // Refresh the window
    glutSwapBuffers();
}

// Idle callback for GLUT
void idle_callback() {
    glutPostRedisplay();
}

// Function to initialize OpenGL
int init_opengl(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("Counter Espionage Agency Simulation");

    // Set up the initial viewport and projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-100.0, 100.0, -100.0, 100.0); // For simulation graphics
    glMatrixMode(GL_MODELVIEW);

    glClearColor(0.05f, 0.05f, 0.05f, 1.0f); // Slightly lighter black background

    // Register callbacks
    glutDisplayFunc(display_callback);
    glutIdleFunc(idle_callback);

    return 0;
}

// Function to render the simulation state
void render(SharedData *shared) {
    global_shared_data = shared;
    glutMainLoop();
}

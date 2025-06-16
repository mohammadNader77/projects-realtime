// visualization.c
#include "shared_memory.h"
#include "config.h"
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// Global pointers for shared memory and semaphore
SharedMemory *shared_data = NULL;
sem_t *sem = NULL;

// Function prototypes
void display();
void timer_func(int value);

// Signal handler for graceful shutdown
void handle_signal(int sig) {
    printf("Visualization received signal %d. Cleaning up and exiting.\n", sig);
    // Perform any necessary cleanup here
    cleanup_shared_memory(shared_data);
    cleanup_semaphore(sem);
    exit(0);
}

// OpenGL display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Set up orthographic projection for 2D rendering
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Fetch data from shared memory
    int generated, processed, moved_processed, moved_unprocessed, moved_backup, deleted;
    sem_wait(sem);
    generated = shared_data->files_generated;
    processed = shared_data->files_processed;
    moved_processed = shared_data->files_moved_to_processed;
    moved_unprocessed = shared_data->files_moved_to_unprocessed;
    moved_backup = shared_data->files_moved_to_backup;
    deleted = shared_data->files_deleted;
    sem_post(sem);

    // Determine the maximum value for scaling
    int max_value = generated;
    if (processed > max_value) max_value = processed;
    if (moved_processed > max_value) max_value = moved_processed;
    if (moved_unprocessed > max_value) max_value = moved_unprocessed;
    if (moved_backup > max_value) max_value = moved_backup;
    if (deleted > max_value) max_value = deleted;
    if (max_value == 0) max_value = 1; // Prevent division by zero

    // Define colors for different counters
    float colors[6][3] = {
        {0.0f, 1.0f, 0.0f},   // Green for Generated
        {0.0f, 0.0f, 1.0f},   // Blue for Processed
        {1.0f, 1.0f, 0.0f},   // Yellow for Moved to Processed
        {1.0f, 0.0f, 0.0f},   // Red for Moved to UnProcessed
        {0.5f, 0.0f, 0.5f},   // Purple for Moved to Backup
        {1.0f, 0.5f, 0.0f}    // Orange for Deleted
    };

    // Draw bars representing different counters
    float bar_width = 100.0f;
    float bar_spacing = 20.0f;
    float base_y = 50.0f;
    float scale = 400.0f / max_value; // Scale to fit within window height

    // Function to draw rectangles (bars) with numerical labels
    void draw_bar(float x, float y, float width, float height, float r, float g, float b, const char *label, int value) {
        // Draw the bar
        glColor3f(r, g, b);
        glBegin(GL_QUADS);
            glVertex2f(x, y);
            glVertex2f(x + width, y);
            glVertex2f(x + width, y + height);
            glVertex2f(x, y + height);
        glEnd();

        // Draw the numerical value above the bar
        glColor3f(1.0f, 1.0f, 1.0f); // White color for text
        glRasterPos2f(x + width / 4, y + height + 5);
        char value_str[20];
        snprintf(value_str, sizeof(value_str), "%d", value);
        for (char *c = value_str; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }

        // Draw the label below the bar
        glRasterPos2f(x, y - 20);
        for (char *c = (char*)label; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
    }

    // Draw each bar with appropriate labels and values
    draw_bar(50, base_y, bar_width, generated * scale, colors[0][0], colors[0][1], colors[0][2], "Generated", generated);
    draw_bar(50 + (bar_width + bar_spacing), base_y, bar_width, processed * scale, colors[1][0], colors[1][1], colors[1][2], "Processed", processed);
    draw_bar(50 + 2*(bar_width + bar_spacing), base_y, bar_width, moved_processed * scale, colors[2][0], colors[2][1], colors[2][2], "Moved Proc", moved_processed);
    draw_bar(50 + 3*(bar_width + bar_spacing), base_y, bar_width, moved_unprocessed * scale, colors[3][0], colors[3][1], colors[3][2], "Moved UnProc", moved_unprocessed);
    draw_bar(50 + 4*(bar_width + bar_spacing), base_y, bar_width, moved_backup * scale, colors[4][0], colors[4][1], colors[4][2], "Moved Backup", moved_backup);
    draw_bar(50 + 5*(bar_width + bar_spacing), base_y, bar_width, deleted * scale, colors[5][0], colors[5][1], colors[5][2], "Deleted", deleted);

    // Draw Legend
    float legend_x = 600.0f;
    float legend_y = 500.0f;
    float legend_box_size = 15.0f;
    float legend_spacing = 20.0f;

    const char *legend_labels[6] = {
        "Generated",
        "Processed",
        "Moved to Processed",
        "Moved to UnProcessed",
        "Moved to Backup",
        "Deleted"
    };

    for (int i = 0; i < 6; i++) {
        // Draw color box
        glColor3f(colors[i][0], colors[i][1], colors[i][2]);
        glBegin(GL_QUADS);
            glVertex2f(legend_x, legend_y - i * legend_spacing);
            glVertex2f(legend_x + legend_box_size, legend_y - i * legend_spacing);
            glVertex2f(legend_x + legend_box_size, legend_y - i * legend_spacing - legend_box_size);
            glVertex2f(legend_x, legend_y - i * legend_spacing - legend_box_size);
        glEnd();

        // Draw label next to the color box
        glColor3f(1.0f, 1.0f, 1.0f); // White color for text
        glRasterPos2f(legend_x + legend_box_size + 5, legend_y - i * legend_spacing - (legend_box_size / 2));
        const char *label = legend_labels[i];
        for (char *c = (char*)label; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
    }

    // Restore projection matrix
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glutSwapBuffers();
}

// OpenGL timer function to update display and check termination
void timer_func(int value) {
    // Redraw the display
    glutPostRedisplay();

    // Register the timer callback again
    glutTimerFunc(1000, timer_func, 0); // Refresh every second
}

int main(int argc, char *argv[]) {
    // Register signal handlers
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    // Initialize shared memory and semaphore
    shared_data = init_shared_memory();
    sem = init_semaphore();

    // Initialize OpenGL (Visualization)
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("File Management Simulation");
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glutDisplayFunc(display);
    glutTimerFunc(1000, timer_func, 0);

    // Start OpenGL main loop
    glutMainLoop();

    // Cleanup (unreachable in current design)
    cleanup_shared_memory(shared_data);
    cleanup_semaphore(sem);

    return 0;
}

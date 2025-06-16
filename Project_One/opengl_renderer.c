// opengl_renderer.c
#include "opengl_renderer.h"
#include "game_shared.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <GL/glut.h>
#include <sys/time.h> 

// Extern variables from main.c
extern int global_pipe_to_player[NUM_TEAMS][NUM_PLAYERS][2];
extern int global_pipe_from_player[NUM_TEAMS][NUM_PLAYERS][2];
extern Team teams[NUM_TEAMS];
extern int game_over;
extern const char *team_names[NUM_TEAMS];
extern float team_colors[NUM_TEAMS][3];
extern int INITIAL_ENERGY;
extern int ENERGY_DECREASE_JUMP;
extern int ENERGY_DECREASE_PULL;
extern int MAX_SCORE;
extern int MAX_TIME;
extern int JUMP_PREP_DELAY;

// OpenGL Window Dimensions
int window_width = 800;
int window_height = 600;

// Visual Representation of players
VisualPlayer visualTeams[NUM_TEAMS][NUM_PLAYERS];

// Physics Structure
typedef struct {
    float velocity;
    float acceleration;
    int isFalling;
} Physics;

Physics physicsTeams[NUM_TEAMS][NUM_PLAYERS];

// Jump Tracker Structure
typedef struct {
    double jump_start_time; // Changed to double for precise time
    int has_jumped; // Flag to indicate if the player has jumped
} JumpTracker;

JumpTracker jumpTrackers[NUM_TEAMS][NUM_PLAYERS];

// Function prototypes for internal use
void display();
void timer(int value);
void handleKeyboard(unsigned char key, int x, int y);
void drawSky();
void drawGround();
void drawBungeeCord(int team_id, int player_id);
void drawEnergyBar(int team_id, int player_id);
void renderBitmapString(float x, float y, void *font, const char *string);
void drawLabels(int team_id, int player_id);
void drawScoreboard();
void updatePhysics();
void updateGameLogic(); 
void cleanup_and_exit();

// Function to get current time in seconds with microsecond precision
double get_current_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Function to initialize OpenGL
void initOpenGL(int argc, char **argv) {
    // Initialize positions and physics
    for (int i = 0; i < NUM_TEAMS; i++) {
        for (int j = 0; j < NUM_PLAYERS; j++) {
            visualTeams[i][j].x = (i - 1) * 200.0f; // Positions: -200, 0, 200
            visualTeams[i][j].y = 240.0f; // Starting y-position (platform level)
            visualTeams[i][j].energy = INITIAL_ENERGY;
            visualTeams[i][j].isJumping = 0;

            // Initialize Physics
            physicsTeams[i][j].velocity = 0.0f;
            physicsTeams[i][j].acceleration = -9.8f; // Gravity
            physicsTeams[i][j].isFalling = 0;

            // Initialize JumpTrackers
            jumpTrackers[i][j].jump_start_time = 0.0;
            jumpTrackers[i][j].has_jumped = 0;
        }
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("Bungee Jumping Game Simulation");
    glClearColor(0.5f, 0.8f, 1.0f, 1.0f); // Sky Blue Background
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-400.0f, 400.0f, 0.0f, 600.0f);
    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);
    glutKeyboardFunc(handleKeyboard);
}

// Function to start the OpenGL main loop
void startOpenGLLoop() {
    glutMainLoop();
}

// Render Bitmap String (for Text Rendering)
void renderBitmapString(float x, float y, void *font, const char *string) {
    glRasterPos2f(x, y);
    while (*string) {
        glutBitmapCharacter(font, *string);
        string++;
    }
}

// Draw Sky with Gradient
void drawSky() {
    glBegin(GL_QUADS);
    // Top color (Light Sky Blue)
    glColor3f(0.529f, 0.808f, 0.922f); // Light Sky Blue
    glVertex2f(-400.0f, 600.0f);
    glVertex2f(400.0f, 600.0f);
    // Bottom color (Deep Sky Blue)
    glColor3f(0.0f, 0.749f, 1.0f); // Deep Sky Blue
    glVertex2f(400.0f, 240.0f);
    glVertex2f(-400.0f, 240.0f);
    glEnd();
}

// Draw Ground
void drawGround() {
    glColor3f(0.133f, 0.545f, 0.133f); // Forest Green
    glBegin(GL_QUADS);
    glVertex2f(-400.0f, 0.0f);
    glVertex2f(400.0f, 0.0f);
    glVertex2f(400.0f, 240.0f);
    glVertex2f(-400.0f, 240.0f);
    glEnd();
}

// Draw Bungee Cord for a Player
void drawBungeeCord(int team_id, int player_id) {
    VisualPlayer *vp = &visualTeams[team_id][player_id];
    glColor3f(0.545f, 0.271f, 0.075f); // Saddle Brown
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(vp->x, 260.0f); // Platform level (adjusted)
    glVertex2f(vp->x, vp->y + 10.0f); // Player position
    glEnd();
    glLineWidth(1.0f); // Reset line width
}

// Draw Energy Bar for a Player
void drawEnergyBar(int team_id, int player_id) {
    VisualPlayer *vp = &visualTeams[team_id][player_id];
    float energy_ratio = vp->energy / (float)INITIAL_ENERGY;
    float bar_width = 20.0f;
    float bar_height = 5.0f;

    // Background Bar
    glColor3f(0.3f, 0.3f, 0.3f); // Gray
    glBegin(GL_QUADS);
    glVertex2f(vp->x - bar_width / 2, vp->y + 25.0f);
    glVertex2f(vp->x + bar_width / 2, vp->y + 25.0f);
    glVertex2f(vp->x + bar_width / 2, vp->y + 25.0f + bar_height);
    glVertex2f(vp->x - bar_width / 2, vp->y + 25.0f + bar_height);
    glEnd();

    // Energy Level
    glColor3f(0.0f, 1.0f, 0.0f); // Green
    glBegin(GL_QUADS);
    glVertex2f(vp->x - bar_width / 2, vp->y + 25.0f);
    glVertex2f(vp->x - bar_width / 2 + (bar_width * energy_ratio), vp->y + 25.0f);
    glVertex2f(vp->x - bar_width / 2 + (bar_width * energy_ratio), vp->y + 25.0f + bar_height);
    glVertex2f(vp->x - bar_width / 2, vp->y + 25.0f + bar_height);
    glEnd();
}

// Draw Labels for a Player
void drawLabels(int team_id, int player_id) {
    VisualPlayer *vp = &visualTeams[team_id][player_id];
    char label[20];
    sprintf(label, "T%sP%d", team_names[team_id], player_id + 1);
    glColor3f(1.0f, 1.0f, 1.0f); // White
    renderBitmapString(vp->x - 10.0f, vp->y + 35.0f, GLUT_BITMAP_HELVETICA_12, label);
}

// Draw Scoreboard
void drawScoreboard() {
    glColor3f(1.0f, 1.0f, 1.0f); // White
    renderBitmapString(-380.0f, 580.0f, GLUT_BITMAP_HELVETICA_18, "Bungee Jumping Game Simulation");

    for (int i = 0; i < NUM_TEAMS; i++) {
        char score_text[50];
        sprintf(score_text, "Team %s: %d", team_names[i], teams[i].score);
        renderBitmapString(-380.0f, 550.0f - (i * 20), GLUT_BITMAP_HELVETICA_12, score_text);
    }
}

// Update Physics for All Players
void updatePhysics() {
    for (int i = 0; i < NUM_TEAMS; i++) {
        for (int j = 0; j < NUM_PLAYERS; j++) {
            if (visualTeams[i][j].isJumping) {
                // Update velocity and position
                physicsTeams[i][j].velocity += physicsTeams[i][j].acceleration * 0.016f; // Assuming 60 FPS (~16ms per frame)
                visualTeams[i][j].y += physicsTeams[i][j].velocity * 0.016f;

                // Simulate bungee cord elasticity (simple collision with ground)
                if (visualTeams[i][j].y <= 50.0f) {
                    physicsTeams[i][j].velocity *= -0.5f; // Dampen velocity
                    visualTeams[i][j].y = 50.0f;
                }

                // Prevent player from going above the platform
                if (visualTeams[i][j].y > 260.0f) { // Adjusted for platform height
                    visualTeams[i][j].y = 260.0f;
                    physicsTeams[i][j].velocity = 0.0f;
                }
            }
        }
    }
}

// Display Callback Function
void display() {
    if (game_over) {
        cleanup_and_exit();
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    // Draw Sky and Ground
    drawSky();
    drawGround();

    // Draw the bungee jumping platform
    glColor3f(0.5f, 0.35f, 0.05f); // Brown
    glBegin(GL_QUADS);
    glVertex2f(-400.0f, 240.0f);
    glVertex2f(400.0f, 240.0f);
    glVertex2f(400.0f, 260.0f);
    glVertex2f(-400.0f, 260.0f);
    glEnd();

    // Draw Scoreboard
    drawScoreboard();

    // Draw Players, Bungee Cords, Energy Bars, and Labels
    for (int i = 0; i < NUM_TEAMS; i++) {
        for (int j = 0; j < NUM_PLAYERS; j++) {
            VisualPlayer *vp = &visualTeams[i][j];
            float energy_ratio = vp->energy / (float)INITIAL_ENERGY;

            // Blend team color with energy ratio for player color
            glColor3f(team_colors[i][0] * energy_ratio,
                      team_colors[i][1] * energy_ratio,
                      team_colors[i][2] * energy_ratio);

            // Draw Player as a Circle
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(vp->x, vp->y);
            for (int angle = 0; angle <= 360; angle += 30) {
                float rad = angle * M_PI / 180.0f;
                glVertex2f(vp->x + cosf(rad) * 10.0f, vp->y + sinf(rad) * 10.0f);
            }
            glEnd();

            // Draw Bungee Cord
            if (vp->isJumping) {
                drawBungeeCord(i, j);
            }

            // Draw Energy Bar
            drawEnergyBar(i, j);

            // Draw Labels
            drawLabels(i, j);
        }
    }

    glutSwapBuffers();
}

// Timer Callback Function
void timer(int value) {
    if (game_over) {
        cleanup_and_exit();
        return;
    }

    updateGameLogic(); // Integrated referee logic here
    updatePhysics();
    glutPostRedisplay();
    // Call timer every 16 ms (~60 FPS)
    glutTimerFunc(16, timer, 0);
}

// Handle Keyboard Inputs
void handleKeyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'p': // Pause functionality can be implemented here
            printf("Pause functionality not implemented.\n");
            fflush(stdout);
            break;
        case 'r': // Reset functionality can be implemented here
            printf("Reset functionality not implemented.\n");
            fflush(stdout);
            break;
        case 27: // ESC key
            game_over = 1; // Set game_over flag to exit
            break;
    }
}

// Update Game Logic (Integrated Referee Logic)
void updateGameLogic() {
    static double start_time = 0.0;
    static int current_player_index = 0;
    static double last_command_time = 0.0;

    if (start_time == 0.0) {
        start_time = get_current_time();
    }

    double elapsed_time = get_current_time() - start_time;
    if (elapsed_time >= MAX_TIME) {
        printf("Maximum time reached. Ending game.\n");
        fflush(stdout);
        game_over = 1;
        return;
    }

    // Check if any team has reached MAX_SCORE
    for (int i = 0; i < NUM_TEAMS; i++) {
        if (teams[i].score >= MAX_SCORE) {
            printf("Team %s has reached the maximum score. Ending game.\n", team_names[i]);
            printf("Winning Team: %s with a score of %d\n", team_names[i], teams[i].score);
            fflush(stdout);
            game_over = 1;
            return;
        }
    }

    // Introduce random preparation delay between jumps (e.g., 1 to 3 seconds)
    int random_prep_delay = rand() % 3 + 1; // Random delay between 1 and 3 seconds

    if (elapsed_time - last_command_time >= (double)(JUMP_PREP_DELAY + random_prep_delay) || last_command_time == 0.0) {
        last_command_time = elapsed_time;

        // Send JUMP command to current player of each team
        for (int i = 0; i < NUM_TEAMS; i++) {
            int player_index = current_player_index;
            char cmd[256];
            sprintf(cmd, "JUMP");
            if (write(global_pipe_to_player[i][player_index][1], cmd, strlen(cmd) + 1) == -1) {
                perror("write JUMP");
            }

            visualTeams[i][player_index].isJumping = 1;
            visualTeams[i][player_index].y = 260.0f; // Reset to platform level
            physicsTeams[i][player_index].velocity = 0.0f;
            physicsTeams[i][player_index].acceleration = -9.8f; // Gravity

            // Record jump start time
            jumpTrackers[i][player_index].jump_start_time = get_current_time();
            jumpTrackers[i][player_index].has_jumped = 1;
        }

        // Read responses from players
        for (int i = 0; i < NUM_TEAMS; i++) {
            char buffer[256];
            int player_index = current_player_index;
            // Expect one response per team
            int n = read(global_pipe_from_player[i][player_index][0], buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                char response[50];
                int value, energy;
                // Handle both two and three token responses
                if (sscanf(buffer, "%s %d %d", response, &value, &energy) >= 2) {
                    if (strcmp(response, "STABILIZED") == 0) {
                        if (jumpTrackers[i][player_index].has_jumped) {
                            double stabilization_time = get_current_time() - jumpTrackers[i][player_index].jump_start_time;
                            // Ensure stabilization_time is positive
                            if (stabilization_time > 0.0) {
                                // Calculate score inversely proportional to stabilization_time
                                // Example: score = K / stabilization_time, where K is a constant
                                // Adjust K as needed to scale scores appropriately
                                double K = 100.0;
                                int score_increment = (int)(K / stabilization_time);
                                if (score_increment < 1) score_increment = 1; // Minimum score increment

                                teams[i].score += score_increment;
                                printf("Player %d from Team %s stabilized in %.2f seconds. Score increment: %d. Total Score: %d\n",
                                       player_index + 1, team_names[i], stabilization_time, score_increment, teams[i].score);
                                fflush(stdout);
                            }
                            jumpTrackers[i][player_index].has_jumped = 0;
                        }
                        visualTeams[i][player_index].energy = energy;
                    } else if (strcmp(response, "INACTIVE") == 0) {
                        visualTeams[i][player_index].energy = 0;
                        printf("Player %d from Team %s is inactive.\n",
                               player_index + 1, team_names[i]);
                        fflush(stdout);
                    }
                }
            } else if (n == 0) {
                printf("Referee: EOF detected on pipe from Team %s Player %d\n",
                       team_names[i], player_index + 1);
                fflush(stdout);
            } else {
                perror("read JUMP response");
            }
        }

        // Send PULL command to other players
        for (int i = 0; i < NUM_TEAMS; i++) {
            int jumper_index = current_player_index;
            for (int j = 0; j < NUM_PLAYERS; j++) {
                if (j != jumper_index) {
                    char cmd[256];
                    sprintf(cmd, "PULL");
                    if (write(global_pipe_to_player[i][j][1], cmd, strlen(cmd) + 1) == -1) {
                        perror("write PULL");
                    }
                }
            }
        }

        // Read responses from pulling players
        for (int i = 0; i < NUM_TEAMS; i++) {
            int jumper_index = current_player_index;
            for (int j = 0; j < NUM_PLAYERS; j++) {
                if (j != jumper_index) {
                    char buffer[256];
                    int n = read(global_pipe_from_player[i][j][0], buffer, sizeof(buffer) - 1);
                    if (n > 0) {
                        buffer[n] = '\0';
                        char response[50];
                        int value, energy;
                        // Handle both two and three token responses
                        if (sscanf(buffer, "%s %d %d", response, &value, &energy) >= 2) {
                            if (strcmp(response, "PULLED") == 0) {
                                visualTeams[i][j].energy = energy;
                                printf("Player %d from Team %s has pulled up jumper. Time taken: %d seconds. Energy: %d\n",
                                       j + 1, team_names[i], value, energy);
                                fflush(stdout);
                            } else if (strcmp(response, "INACTIVE") == 0) {
                                visualTeams[i][j].energy = 0;
                                printf("Player %d from Team %s is inactive.\n",
                                       j + 1, team_names[i]);
                                fflush(stdout);
                            }
                        }
                    } else if (n == 0) {
                        printf("Referee: EOF detected on pipe from Team %s Player %d\n",
                               team_names[i], j + 1);
                        fflush(stdout);
                    } else {
                        perror("read PULL response");
                    }
                }
            }
        }


        current_player_index = (current_player_index + 1) % NUM_PLAYERS;
    }
}

// Cleanup and exit function
void cleanup_and_exit() {
    printf("Cleaning up and exiting game.\n");
    fflush(stdout);
    exit(0);
}

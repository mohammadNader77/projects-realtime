// main.c
#include "game_shared.h"
#include "opengl_renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

// Global Variables
const char *team_names[NUM_TEAMS] = {"Team_A", "Team_B", "Team_C"};

// Team Colors (Red, Green, Blue)
float team_colors[NUM_TEAMS][3] = {
    {1.0f, 0.0f, 0.0f}, // Team A - Red
    {0.0f, 1.0f, 0.0f}, // Team B - Green
    {0.0f, 0.0f, 1.0f}  // Team C - Blue
};

Team teams[NUM_TEAMS];

// Game Settings
int INITIAL_ENERGY;
int ENERGY_DECREASE_JUMP;
int ENERGY_DECREASE_PULL;
int ENERGY_DECREASE_TIME;
int MAX_SCORE;
int MAX_TIME;
int JUMP_PREP_DELAY;

// Game Over Flag
int game_over = 0;

// Function Prototypes
void read_settings(char *filename);
void initialize_game();
void player_process(Player *player, int team_id, int player_id, int pipe_to_player[2], int pipe_from_player[2]);
void handle_sigterm(int sig);

// Global Pipes for Communication
int global_pipe_to_player[NUM_TEAMS][NUM_PLAYERS][2];
int global_pipe_from_player[NUM_TEAMS][NUM_PLAYERS][2];

void read_settings(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Error opening settings file");
        exit(EXIT_FAILURE);
    }
    char line[256];
    char key[50];
    int value;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%s %d", key, &value) == 2) {
            if (strcmp(key, "INITIAL_ENERGY") == 0) INITIAL_ENERGY = value;
            else if (strcmp(key, "ENERGY_DECREASE_JUMP") == 0) ENERGY_DECREASE_JUMP = value;
            else if (strcmp(key, "ENERGY_DECREASE_PULL") == 0) ENERGY_DECREASE_PULL = value;
            else if (strcmp(key, "ENERGY_DECREASE_TIME") == 0) ENERGY_DECREASE_TIME = value;
            else if (strcmp(key, "MAX_SCORE") == 0) MAX_SCORE = value;
            else if (strcmp(key, "MAX_TIME") == 0) MAX_TIME = value;
            else if (strcmp(key, "JUMP_PREP_DELAY") == 0) JUMP_PREP_DELAY = value; // Read JUMP_PREP_DELAY
        }
    }
    fclose(fp);
}

void initialize_game() {
    for (int i = 0; i < NUM_TEAMS; i++) {
        teams[i].team_id = i;
        teams[i].score = 0;
        for (int j = 0; j < NUM_PLAYERS; j++) {
            teams[i].players[j].team_id = i;
            teams[i].players[j].player_id = j;
            teams[i].players[j].energy = INITIAL_ENERGY;
            teams[i].players[j].pid = -1;
        }
    }
}

void player_process(Player *player, int team_id, int player_id, int pipe_to_player[2], int pipe_from_player[2]) {
    // Close unused pipe ends in player
    close(pipe_to_player[1]);   // Close write end in player
    close(pipe_from_player[0]); // Close read end in player

    // Seed random number generator uniquely for each player
    srand(time(NULL) ^ (getpid() << 16));


    char buffer[256]; // Buffer for reading commands

    while (1) {
        memset(buffer, 0, sizeof(buffer)); // Clear buffer before reading
        int n = read(pipe_to_player[0], buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            char command[50];
            sscanf(buffer, "%s", command);
            if (strcmp(command, "JUMP") == 0) {
                // Check if player has energy
                if (player->energy <= 0) {
                    sprintf(buffer, "INACTIVE %d", player_id + 1);
                    write(pipe_from_player[1], buffer, strlen(buffer) + 1);
                    continue;
                }

                int stabilization_time = (ENERGY_DECREASE_JUMP > 0) ? (rand() % ENERGY_DECREASE_JUMP + 1) : 1;
                sleep(stabilization_time);

                // Decrease energy
                player->energy -= ENERGY_DECREASE_JUMP;
                if (player->energy < 0) player->energy = 0;

                // Send response back to referee
                sprintf(buffer, "STABILIZED %d %d", stabilization_time, player->energy);
                write(pipe_from_player[1], buffer, strlen(buffer) + 1);

                if (player->energy <= 0) {
                    sprintf(buffer, "INACTIVE %d", player_id + 1);
                    write(pipe_from_player[1], buffer, strlen(buffer) + 1);
                    break;
                }
            } else if (strcmp(command, "PULL") == 0) {
                if (player->energy <= 0) {
                    sprintf(buffer, "INACTIVE %d", player_id + 1);
                    write(pipe_from_player[1], buffer, strlen(buffer) + 1);
                    continue;
                }

                int pull_time = (ENERGY_DECREASE_PULL > 0) ? (rand() % ENERGY_DECREASE_PULL + 1) : 1;
                sleep(pull_time);

                player->energy -= ENERGY_DECREASE_PULL;
                if (player->energy < 0) player->energy = 0;

                // Send response back to referee
                sprintf(buffer, "PULLED %d %d", pull_time, player->energy);
                write(pipe_from_player[1], buffer, strlen(buffer) + 1);

                if (player->energy <= 0) {
                    sprintf(buffer, "INACTIVE %d", player_id + 1);
                    write(pipe_from_player[1], buffer, strlen(buffer) + 1);
                    break;
                }
            }
        } else {
            if (n == 0) {
                break;
            } else {
                perror("read");
                exit(EXIT_FAILURE);
            }
        }
    }

    close(pipe_to_player[0]);
    close(pipe_from_player[1]);

    exit(EXIT_SUCCESS);
}

void handle_sigterm(int sig) {
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <settings_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    read_settings(argv[1]);
    initialize_game();

    srand(time(NULL));

    int pipe_to_player_local[NUM_TEAMS][NUM_PLAYERS][2];
    int pipe_from_player_local[NUM_TEAMS][NUM_PLAYERS][2];

    for (int i = 0; i < NUM_TEAMS; i++) {
        for (int j = 0; j < NUM_PLAYERS; j++) {
            if (pipe(pipe_to_player_local[i][j]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            if (pipe(pipe_from_player_local[i][j]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }
    }

    pid_t referee_pid = fork();
    if (referee_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (referee_pid == 0) {
        // Referee process
        // Close unused pipe ends
        for (int i = 0; i < NUM_TEAMS; i++) {
            for (int j = 0; j < NUM_PLAYERS; j++) {
                close(pipe_to_player_local[i][j][0]);    // Close read end
                close(pipe_from_player_local[i][j][1]);  // Close write end
            }
        }

        // Copy pipes to global variables
        memcpy(global_pipe_to_player, pipe_to_player_local, sizeof(pipe_to_player_local));
        memcpy(global_pipe_from_player, pipe_from_player_local, sizeof(pipe_from_player_local));

        // Initialize OpenGL and start the main loop
        initOpenGL(argc, argv);
        startOpenGLLoop();

        exit(EXIT_SUCCESS);
    } else {
        // Parent process (fork players)
        for (int i = 0; i < NUM_TEAMS; i++) {
            for (int j = 0; j < NUM_PLAYERS; j++) {
                pid_t pid = fork();
                if (pid == -1) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                } else if (pid == 0) {
                    // Player process
                    signal(SIGTERM, handle_sigterm);

                    // Close unused pipes
                    for (int k = 0; k < NUM_TEAMS; k++) {
                        for (int l = 0; l < NUM_PLAYERS; l++) {
                            if (!(k == i && l == j)) {
                                close(pipe_to_player_local[k][l][0]);
                                close(pipe_to_player_local[k][l][1]);
                                close(pipe_from_player_local[k][l][0]);
                                close(pipe_from_player_local[k][l][1]);
                            }
                        }
                    }
                    close(pipe_to_player_local[i][j][1]);    // Close write end in player
                    close(pipe_from_player_local[i][j][0]);  // Close read end in player

                    player_process(&teams[i].players[j], i, j, pipe_to_player_local[i][j], pipe_from_player_local[i][j]);
                    exit(EXIT_SUCCESS);
                } else {
                    // Parent process records player PID
                    teams[i].players[j].pid = pid;
                }
            }
        }

        // Parent process waits for referee process
        int status;
        waitpid(referee_pid, &status, 0);

        // Terminate player processes
        for (int i = 0; i < NUM_TEAMS; i++) {
            for (int j = 0; j < NUM_PLAYERS; j++) {
                if (kill(teams[i].players[j].pid, 0) == 0) {
                    kill(teams[i].players[j].pid, SIGTERM);
                    waitpid(teams[i].players[j].pid, NULL, 0);
                }
            }
        }

        printf("Game over\n");
    }

    return 0;
}

// game_shared.h
#ifndef GAME_SHARED_H
#define GAME_SHARED_H

#include <unistd.h>
#include <sys/types.h>

#define NUM_TEAMS 3
#define NUM_PLAYERS 3

// Changed JUMP_PREP_DELAY from macro to external variable
extern int JUMP_PREP_DELAY;

// Team names and colors
extern const char *team_names[NUM_TEAMS];
extern float team_colors[NUM_TEAMS][3];

// Game settings
extern int INITIAL_ENERGY;
extern int ENERGY_DECREASE_JUMP;
extern int ENERGY_DECREASE_PULL;
extern int ENERGY_DECREASE_TIME;
extern int MAX_SCORE;
extern int MAX_TIME;

// Player Structure
typedef struct {
    int team_id;
    int player_id;
    int energy;
    pid_t pid;
} Player;

// Team Structure
typedef struct {
    int team_id;
    Player players[NUM_PLAYERS];
    int score;
} Team;

// Extern variable for teams
extern Team teams[NUM_TEAMS];

// Game Over Flag
extern int game_over;

#endif 

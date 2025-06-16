// config.h
#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int agency_members;
    int group_creation_interval;
    int group_size_min;
    int group_size_max;
    int military_group_percentage;
    double spy_infiltration_probability;
    double target_probability;
    int recovery_light;
    int recovery_severe;
    double suspicion_threshold;
    double arrest_release_threshold;
    double arrest_imprison_threshold;
    int max_killed;
    int max_injured;
    int agency_time_limit;
} Config;

// Function to parse the configuration file
int parse_config(const char *filename, Config *config);

#endif

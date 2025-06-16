// config.h
#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int num_generators;
    int num_calculators;
    int inspectors_type1;
    int inspectors_type2;
    int inspectors_type3;
    int gen_interval_min;
    int gen_interval_max;
    int rows_min;
    int rows_max;
    int columns_min;
    int columns_max;
    float value_min;
    float value_max;
    float missing_percentage;
    int threshold_files_processed;
    int threshold_files_not_processed;
    int threshold_files_backup;
    int threshold_files_deleted;
    int runtime_limit_minutes;
    int type1_threshold_age; // Age threshold for Type1 Inspectors in seconds
} Config;

// Function prototype
void parse_config(const char *filename, Config *config);

#endif // CONFIG_H

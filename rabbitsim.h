#ifndef RABBITSIM_H
#define RABBITSIM_H

// Standard library includes
#include <stddef.h>     // For size_t
#include <stdint.h>     // Required for PCG's uint types like uint32_t, uint64_t
#include <stdio.h>      // For standard input/output functions like printf, fflush
#include <stdlib.h>     // For general utilities like malloc, realloc, free
#include <stdarg.h>     // For variable argument lists (va_list, va_start, etc.)
#include <time.h>       // For time-related functions, often used for seeding random number generators
#include <math.h>       // For mathematical functions like exp, sqrt
#include <omp.h>        // For OpenMP parallel programming directives
#include "pcg_basic.h"  // Include the PCG (Permuted Congruential Generator) library header for random numbers
#include <math.h>       // For math functions

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Define initial capacity for rabbit array to avoid frequent reallocations
#define INIT_RABIT_CAPACITY 1000000 

// Survival rates for rabbits at different life stages. These values are crucial
// for the long-term stability or extinction of the simulated population.
// For example, (75.6, 94.6) might lead to a stable population for a period
// but eventually extinction over a very long time.
#define INIT_SRV_RATE 91.63f  // Initial survival rate for newborn rabbits
#define ADULT_SRV_RATE 95.83f // Survival rate for adult rabbits

// Survival calculation methods
typedef enum {
    SURVIVAL_STATIC,    // Constant values (default)
    SURVIVAL_GAUSSIAN,  // Gaussian distribution
    SURVIVAL_EXPONENTIAL // Exponential distribution
} survival_method_t;

// Global variable to define survival calculation method
extern survival_method_t survival_method;

// Macro to control output printing. If PRINT_OUTPUT is non-zero, logs will be printed.
#define PRINT_OUTPUT 1

// NEW: Macro to control data logging to files
#define ENABLE_DATA_LOGGING 1

// NEW: Maximum number of simulations to log detailed monthly data (to avoid huge files)
#define MAX_SIMULATIONS_TO_LOG 3

// Conditional compilation for logging messages.
// If PRINT_OUTPUT is enabled, LOG_PRINT will call printf and fflush.
// Otherwise, it will expand to an empty operation, effectively removing log calls from the compiled code.
#if defined(PRINT_OUTPUT) && PRINT_OUTPUT != 0
    #define LOG_PRINT(format, ...) do { printf(format, ##__VA_ARGS__); fflush(stdout); } while (0)
#else
    #define LOG_PRINT(format, ...) do { } while (0)
#endif

// Structure representing a single rabbit in the simulation.
typedef struct rabbit {
    int sex;                     // 0 for female, 1 for male
    int status;                  // 0 for dead, 1 for alive
    int age;                     // Age in months
    int mature;                  // 0 for immature, 1 for mature (can reproduce)
    int maturity_age;            // The age at which the rabbit became mature
    int pregnant;                // 0 for not pregnant, 1 for pregnant (only for females)
    int nb_litters_y;            // Number of litters a female can have per year
    int nb_litters;              // Number of litters a female has had in the current year
    float survival_rate;         // Probability of survival for the current month (e.g., 94.6 for 94.6% chance)
    int survival_check_flag;     // Flag to indicate if survival has already been checked this month (1 for checked, 0 for not) // Not used currently since i check each month instead of each year so maybe i should remove it later and update the comment related to it
} s_rabbit; // Alias for the rabbit structure

// NEW: Structure to store monthly statistics for a single simulation
typedef struct {
    int month;                   // Month number
    int total_alive;             // Total living rabbits
    int males;                   // Living males
    int females;                 // Living females
    int mature_rabbits;          // Number of mature rabbits
    int pregnant_females;        // Number of pregnant females
    int births_this_month;       // Number of births this month
    int deaths_this_month;       // Number of deaths this month
    float avg_age;               // Average age of living rabbits
} s_monthly_stats;

// Structure representing a single simulation instance.
typedef struct {
    s_rabbit *rabbits;           // Pointer to a dynamically allocated array of rabbits
    size_t rabbit_count;         // Current number of rabbits in the array (both alive and dead but still in array)
    size_t dead_rabbit_count;    // Total number of rabbits that have died throughout the simulation
    size_t rabbit_capacity;      // Current allocated capacity for the rabbits array
    int *free_indices;           // Array of indices of "dead" rabbit slots that can be reused
    size_t free_count;           // Number of available free slots
    int sex_distribution[2];     // Distribution of the males and the females
    
    // NEW: Logging-related fields
    s_monthly_stats *monthly_data;  // Array to store monthly statistics
    int monthly_data_capacity;      // Allocated capacity for monthly_data
    int monthly_data_count;         // Number of months recorded
    int deaths_this_month;          // Track deaths for current month
    int births_this_month;          // Track births for current month
} s_simulation_instance; // Alias for the simulation instance structure

/**
 * @brief Structure to store comprehensive results from a single simulation run.
 * 
 * This structure captures various statistics about the simulation, including
 * population dynamics, sex distribution, and temporal patterns. It replaces
 * the previous float array approach for better type safety and clarity.
 */
typedef struct {
    int total_dead;              // Total number of rabbit deaths throughout simulation
    int final_alive;             // Number of living rabbits at simulation end
    int extinction_month;        // Month when population went extinct (0 if no extinction)
    int final_males;             // Number of males at simulation end
    int final_females;           // Number of females at simulation end
    int peak_population;         // Maximum living population reached during simulation
    int peak_population_month;   // Month when peak population was achieved
    int min_population;          // Minimum living population after initial population (excludes month 0)
    int min_population_month;    // Month when minimum population occurred
    long long total_population_sum; // Sum of population across all months (for average calculation)
    int months_simulated;        // Actual number of months simulated (may be less than requested if extinction)
    float male_percentage;       // Percentage of males in final population
    float female_percentage;     // Percentage of females in final population
} s_simulation_results;


//   > Function Prototypes <
// Declarations for all functions used in the rabbit simulation.
// Many functions now include a 'pcg32_random_t* rng' parameter
// to pass the random number generator state explicitly,
// allowing for thread-safe and reproducible simulations.

int fibonacci(int n);
double genrand_real(pcg32_random_t* rng);

int ensure_capacity(s_simulation_instance *sim);
void add_rabbit(s_simulation_instance *sim, int is_mature, float init_srv_rate, int age, int sex);
void init_2_super_rabbits(s_simulation_instance *sim);
void init_starting_population(s_simulation_instance *sim, int nb_rabbits, pcg32_random_t* rng);
void reset_population(s_simulation_instance *sim);

int generate_sex(pcg32_random_t* rng);
int generate_random_age(pcg32_random_t *rng);
int check_maturity(int age, pcg32_random_t* rng);
void update_maturity(s_simulation_instance *sim, size_t i, pcg32_random_t* rng);

int check_survival_rate(s_simulation_instance *sim, size_t i, pcg32_random_t *rng);
void kill_rabbit(s_simulation_instance *sim, size_t i);
void check_survival(s_simulation_instance *sim, size_t i, pcg32_random_t* rng);
void update_survival_rate(s_simulation_instance *sim, size_t i, pcg32_random_t *rng);

float calculate_base_survival_rate(s_simulation_instance *sim, size_t i);
float calculate_survival_rate_static(float base_rate);
float calculate_survival_rate_gaussian(float base_rate, pcg32_random_t *rng);
float calculate_survival_rate_exponential(float base_rate, pcg32_random_t *rng);

int generate_litters_per_year(pcg32_random_t* rng);
void update_litters_per_year(s_simulation_instance *sim, size_t i, pcg32_random_t* rng);
int can_be_pregnant_this_month(s_simulation_instance *sim, size_t i, pcg32_random_t* rng);
int give_birth(s_simulation_instance *sim, size_t i, pcg32_random_t* rng);
void check_pregnancy(s_simulation_instance *sim, size_t i, pcg32_random_t* rng);
void create_new_generation(s_simulation_instance *sim, int nb_new_born, pcg32_random_t* rng);

void update_rabbits(s_simulation_instance *sim, pcg32_random_t* rng);
s_simulation_results simulate(s_simulation_instance *sim, int months, int initial_population_nb, pcg32_random_t* rng);
void multi_simulate(int months, int initial_population_nb, int nb_simulation, uint64_t base_seed);

// NEW: Logging function prototypes
void init_monthly_logging(s_simulation_instance *sim, int months);
void record_monthly_stats(s_simulation_instance *sim, int month);
void write_simulation_log(s_simulation_instance *sim, int sim_number, int initial_population);
void write_summary_log(int months, int initial_population, int nb_simulations, 
                       s_simulation_results *all_results, uint64_t base_seed);

#endif
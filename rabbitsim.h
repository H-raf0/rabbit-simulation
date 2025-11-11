#ifndef RABBITSIM_H
#define RABBITSIM_H

// Standard library includes
#include <stddef.h>     // For size_t
#include <stdint.h>     // Required for PCG's uint types like uint32_t, uint64_t
#include <stdio.h>      // For standard input/output functions like printf, fflush
#include <stdlib.h>     // For general utilities like malloc, realloc, free
#include <stdarg.h>     // For variable argument lists (va_list, va_start, etc.)
#include <time.h>       // For time-related functions, often used for seeding random number generators
#include <omp.h>        // For OpenMP parallel programming directives
#include "pcg_basic.h"  // Include the PCG (Permuted Congruential Generator) library header for random numbers

// Define initial capacity for rabbit array to avoid frequent reallocations
#define INIT_RABIT_CAPACITY 1000000 

// Survival rates for rabbits at different life stages. These values are crucial
// for the long-term stability or extinction of the simulated population.
// For example, (75.6, 94.6) might lead to a stable population for a period
// but eventually extinction over a very long time.
#define INIT_SRV_RATE 75.6f  // Initial survival rate for newborn rabbits
#define ADULT_SRV_RATE 94.6f // Survival rate for adult rabbits

// Macro to control output printing. If PRINT_OUTPUT is non-zero, logs will be printed.
#define PRINT_OUTPUT 0

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
    int survival_check_flag;     // Flag to indicate if survival has already been checked this month (1 for checked, 0 for not)
} s_rabbit; // Alias for the rabbit structure

// Structure representing a single simulation instance.
typedef struct {
    s_rabbit *rabbits;           // Pointer to a dynamically allocated array of rabbits
    size_t rabbit_count;         // Current number of rabbits in the array (both alive and dead but still in array)
    size_t dead_rabbit_count;    // Total number of rabbits that have died throughout the simulation
    size_t rabbit_capacity;      // Current allocated capacity for the rabbits array
    int *free_indices;           // Array of indices of "dead" rabbit slots that can be reused
    size_t free_count;           // Number of available free slots
} s_simulation_instance; // Alias for the simulation instance structure
// structures de stats 
typedef struct {
    double alive_final;   // nb de lapins vivants à la fin
    double dead_final;    // nb total de lapins morts
    int    extinct;       // 1 si tout le monde est mort, 0 sinon
    int    months_run;    // nb de mois réellement simulés
} s_sim_result;

typedef struct {
    double sum_alive;
    double sumsq_alive;
    double min_alive;
    double max_alive;

    double sum_dead;
    double sumsq_dead;
    double min_dead;
    double max_dead;

    int    nb_extinctions;
    int    n;             // nombre de simulations accumulées
} s_stats_acc;

//   > Function Prototypes <
// Declarations for all functions used in the rabbit simulation.
// Many functions now include a 'pcg32_random_t* rng' parameter
// to pass the random number generator state explicitly,
// allowing for thread-safe and reproducible simulations.

int fibonacci(int n);
double genrand_real(pcg32_random_t* rng);

int ensure_capacity(s_simulation_instance *sim);
void add_rabbit(s_simulation_instance *sim, pcg32_random_t* rng, int is_mature, float init_srv_rate, int age);
void init_2_super_rabbits(s_simulation_instance *sim, pcg32_random_t* rng);
void init_starting_population(s_simulation_instance *sim, int nb_rabbits, pcg32_random_t* rng);
void reset_population(s_simulation_instance *sim);

int generate_sex(pcg32_random_t* rng);
int generate_random_age(pcg32_random_t *rng);
int check_maturity(int age, pcg32_random_t* rng);
void update_maturity(s_simulation_instance *sim, size_t i, pcg32_random_t* rng);

int check_survival_rate(s_simulation_instance *sim, size_t i, pcg32_random_t *rng);
void kill_rabbit(s_simulation_instance *sim, size_t i);
void check_survival(s_simulation_instance *sim, size_t i, pcg32_random_t* rng);
void update_survival_rate(s_simulation_instance *sim, size_t i);

int generate_litters_per_year(pcg32_random_t* rng);
void update_litters_per_year(s_simulation_instance *sim, size_t i, pcg32_random_t* rng);
int can_be_pregnant_this_month(s_simulation_instance *sim, size_t i, pcg32_random_t* rng);
int give_birth(s_simulation_instance *sim, size_t i, pcg32_random_t* rng);
void check_pregnancy(s_simulation_instance *sim, size_t i, pcg32_random_t* rng);
void create_new_generation(s_simulation_instance *sim, int nb_new_born, pcg32_random_t* rng);

void update_rabbits(s_simulation_instance *sim, pcg32_random_t* rng);
float* simulate(s_simulation_instance *sim, int months, int initial_population_nb, pcg32_random_t* rng);
float* stock_data(int count, ...);
void multi_simulate(int months, int initial_population_nb, int nb_simulation, uint64_t base_seed);
//stats 
// init / ajout / fusion / affichage
void stats_acc_init(s_stats_acc *st);
void stats_acc_add(s_stats_acc *st, const s_sim_result *r);
void stats_acc_merge(s_stats_acc *dst, const s_stats_acc *src);
void stats_print_report(const s_stats_acc *st, int months, int initial_population, int nb_simulation);
s_sim_result simulate_stats(s_simulation_instance *sim, int months, int initial_population_nb, pcg32_random_t *rng);
#endif
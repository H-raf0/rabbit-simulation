#ifndef RABBITSIM_H
#define RABBITSIM_H

#include <stddef.h> // for size_t
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <omp.h>

#define INIT_RABIT_CAPACITY 1000000
#define INIT_SRV_RATE 35

// --- START: NEW MACRO DEFINITION ---

// Set PRINT_OUTPUT to 1 to see all simulation steps, or 0 to hide them.
#define PRINT_OUTPUT 1

#if defined(PRINT_OUTPUT) && PRINT_OUTPUT != 0
    // If PRINT_OUTPUT is 1, our macro becomes a printf call followed by a flush.
    #define LOG_PRINT(format, ...) do { printf(format, ##__VA_ARGS__); fflush(stdout); } while (0)
#else
    // If PRINT_OUTPUT is 0, our macro does absolutely nothing.
    #define LOG_PRINT(format, ...) do { } while (0)
#endif

// --- END: NEW MACRO DEFINITION ---

typedef struct rabbit {
    char sex;
    int status;
    int age;
    int mature;
    int maturity_age;
    int pregnant;
    int nb_litters_y;
    int nb_litters;
    int survival_rate;
} s_rabbit;

// This struct holds all the data for a single, independent simulation instance.
// This makes the simulation thread-safe.
typedef struct {
    s_rabbit *rabbits;
    size_t rabbit_count;
    size_t dead_rabbit_count;
    size_t rabbit_capacity;
    int *free_indices;
    size_t free_count;
} s_simulation_instance;

// --- function prototypes ---
int fibonacci(int n);

int ensure_capacity(s_simulation_instance *sim);
void init_2_super_rabbits(s_simulation_instance *sim);
void init_starting_population(s_simulation_instance *sim, int nb_rabbits);
void reset_population(s_simulation_instance *sim);
void add_rabbit(s_simulation_instance *sim);
char generate_sex(void);
int check_maturity(int age);
void update_maturity(s_simulation_instance *sim, size_t i);
void check_survival(s_simulation_instance *sim, size_t i);
int check_survival_rate(int survival_rate);
void update_survival_rate(s_simulation_instance *sim, size_t i);
void kill_rabbit(s_simulation_instance *sim, size_t i);
int generate_litters_per_year(void);
void update_litters_per_year(s_simulation_instance *sim, size_t i);
int can_be_pregnant_this_month(s_simulation_instance *sim, size_t i);
int give_birth(s_simulation_instance *sim, size_t i);
void check_pregnancy(s_simulation_instance *sim, size_t i);
void create_new_generation(s_simulation_instance *sim, int nb_new_born);
void update_rabbits(s_simulation_instance *sim);
float* simulate(s_simulation_instance *sim, int months, int initial_population_nb);
float* stock_data(int count, ...);
void multi_simulate(int months, int initial_population_nb, int nb_simulation);

#endif

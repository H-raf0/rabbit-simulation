#ifndef RABBITSIM_H
#define RABBITSIM_H

#include <stddef.h>
#include <stdint.h>     // Required for PCG's uint types
#include "pcg_basic.h"  // Include the PCG library header

#define INIT_RABIT_CAPACITY 1000000
#define INIT_SRV_RATE 35
#define ADULT_SRV_RATE 60

#define PRINT_OUTPUT 1

#if defined(PRINT_OUTPUT) && PRINT_OUTPUT != 0
    #define LOG_PRINT(format, ...) do { printf(format, ##__VA_ARGS__); fflush(stdout); } while (0)
#else
    #define LOG_PRINT(format, ...) do { } while (0)
#endif

typedef struct rabbit {
    int sex; // 0 male 1 female
    int status;
    int age;
    int mature;
    int maturity_age;
    int pregnant;
    int nb_litters_y;
    int nb_litters;
    int survival_rate;
    int survival_check_flag; // 1 checked, 0 no
} s_rabbit;

typedef struct {
    s_rabbit *rabbits;
    size_t rabbit_count;
    size_t dead_rabbit_count;
    size_t rabbit_capacity;
    int *free_indices;
    size_t free_count;
} s_simulation_instance;


// --- Function Prototypes ---
// Note the new pcg32_random_t* rng parameter in many functions.

int fibonacci(int n);
double genrand_real(pcg32_random_t* rng);

int ensure_capacity(s_simulation_instance *sim);
void add_rabbit(s_simulation_instance *sim, pcg32_random_t* rng, int is_mature, int init_srv_rate);
void init_2_super_rabbits(s_simulation_instance *sim, pcg32_random_t* rng);
void init_starting_population(s_simulation_instance *sim, int nb_rabbits, pcg32_random_t* rng);
void reset_population(s_simulation_instance *sim);

int generate_sex(pcg32_random_t* rng);
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

#endif
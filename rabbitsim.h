#ifndef RABBITSIM_H
#define RABBITSIM_H

#include <stddef.h> // for size_t

#define INIT_RABIT_CAPACITY 1000000
#define INIT_SRV_RATE 35

// --- START: NEW MACRO DEFINITION ---

// Set PRINT_OUTPUT to 1 to see all simulation steps, or 0 to hide them.
#define PRINT_OUTPUT 0

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

extern s_rabbit *rabbits;
extern size_t rabbit_count;
extern int current_month;
extern size_t rabbit_capacity;
extern int *free_indices;
extern size_t free_count;

// --- function prototypes ---
int fibonacci(int n);

int ensure_capacity(void);
void init_starting_population(int nb_rabbits);
void reset_population(void);
void add_rabbit(void);
char generate_sex(void);
int check_maturity(int age);
void update_maturity(size_t i);
void check_survival(size_t i);
int check_survival_rate(int survival_rate);
void update_survival_rate(size_t i);
void kill_rabbit(size_t i);
int generate_litters_per_year(void);
void update_litters_per_year(size_t i);
int can_be_pregnant_this_month(size_t i);
int give_birth(size_t i);
void check_pregnancy(size_t i);
void create_new_generation(int nb_new_born);
void update_rabbits();
float* simulate(int months, int initial_population_nb);
float* stock_data(int count, ...);
void multi_simulate(int months, int initial_population_nb, int nb_simulation);

#endif

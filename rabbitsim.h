#ifndef RABBITSIM_H
#define RABBITSIM_H

#include <stddef.h> // for size_t

#define MAX_RABBITS 100000

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
extern int free_indices[MAX_RABBITS];
extern size_t free_count;

// --- function prototypes ---
int fibonacci(int n);

void init_population(void);
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
void update_rabbits(int current_month);
void simulate(int months, int initial_population_nb);
void clear_screen(void);

#endif

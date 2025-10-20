#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <omp.h>

#include "rabbitsim.h"
#include "mt19937ar.h"

// All global state variables have been moved into the s_simulation_instance struct.

int fibonacci(int n)
{
    if (n == 0 || n == 1)
    {
        return 2;
    }
    else
    {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}

int ensure_capacity(s_simulation_instance *sim)
{
    if (sim->rabbit_count < sim->rabbit_capacity)
    {
        return 1;
    }

    size_t new_capacity = (sim->rabbit_capacity == 0) ? INIT_RABIT_CAPACITY : sim->rabbit_capacity * 2;
    
    s_rabbit *temp_rabbits = realloc(sim->rabbits, sizeof(s_rabbit) * new_capacity);
    if (temp_rabbits == NULL)
    {
        LOG_PRINT("\n    MEMORY REALLOCATION FAILED (rabbits)!\n");
        return 0;
    }
    sim->rabbits = temp_rabbits;

    int *temp_indices = realloc(sim->free_indices, sizeof(int) * new_capacity);
    if (temp_indices == NULL)
    {
        LOG_PRINT("\n    MEMORY REALLOCATION FAILED (free_indices)!\n");
        return 0;
    }
    sim->free_indices = temp_indices;
    
    sim->rabbit_capacity = new_capacity;
    return 1;
}

void add_rabbit(s_simulation_instance *sim)
{
    if (!ensure_capacity(sim))
    {
        return;
    }
    s_rabbit *r;

    if (sim->free_count > 0)
    {
        size_t index = sim->free_indices[--sim->free_count];
        r = &sim->rabbits[index];
    }
    else
    {
        r = &sim->rabbits[sim->rabbit_count++];
    }

    r->sex = generate_sex();
    r->status = 1;
    r->age = 0;
    r->mature = 0;
    r->maturity_age = 0;
    r->pregnant = 0;
    r->nb_litters_y = 0;
    r->nb_litters = 0;
    r->survival_rate = INIT_SRV_RATE;
}

void init_2_super_rabbits(s_simulation_instance *sim)
{
    add_rabbit(sim);
    add_rabbit(sim);
    sim->rabbits[0].sex = 'M';
    sim->rabbits[0].mature = 1;
    sim->rabbits[0].survival_rate = 100;
    sim->rabbits[1].sex = 'F';
    sim->rabbits[1].mature = 1;
    sim->rabbits[1].survival_rate = 100;
}

void init_starting_population(s_simulation_instance *sim, int nb_rabbits)
{
    for (int i = 0; i < nb_rabbits; ++i)
    {
        add_rabbit(sim);
    }
}

void reset_population(s_simulation_instance *sim)
{
    free(sim->rabbits);
    sim->rabbits = NULL;
    free(sim->free_indices);
    sim->free_indices = NULL;

    sim->rabbit_count = 0;
    sim->free_count = 0;
    sim->dead_rabbit_count = 0;
    sim->rabbit_capacity = 0;
}

char generate_sex()
{
    return (genrand_real1() * 100 <= 50) ? 'M' : 'F';
}

int check_maturity(int age)
{
    float chance = (float)age / 8.0f;
    return (genrand_real1() <= chance) ? 1 : 0;
}

void update_maturity(s_simulation_instance *sim, size_t i)
{
    if (sim->rabbits[i].mature) return;

    if (sim->rabbits[i].age >= 5 && sim->rabbits[i].mature == 0)
    {
        if (check_maturity(sim->rabbits[i].age))
        {
            sim->rabbits[i].mature = 1;
            sim->rabbits[i].maturity_age = sim->rabbits[i].age;
        }
    }
}

int check_survival_rate(int survival_rate)
{
    if (survival_rate < 0) return 1;
    return (genrand_real1() * 100 <= survival_rate) ? 1 : 0;
}

void kill_rabbit(s_simulation_instance *sim, size_t i)
{
    sim->rabbits[i].status = 0;
    sim->free_indices[sim->free_count++] = i;
    sim->dead_rabbit_count++;
}

void check_survival(s_simulation_instance *sim, size_t i)
{
    if (sim->rabbits[i].status == 1)
    {
        if (!check_survival_rate(sim->rabbits[i].survival_rate))
        {
            kill_rabbit(sim, i);
        }
        else
        {
            sim->rabbits[i].survival_rate *= -1;
        }
    }
}

void update_survival_rate(s_simulation_instance *sim, size_t i)
{
    if (sim->rabbits[i].age % 12 == 0)
    {
        sim->rabbits[i].survival_rate = sim->rabbits[i].survival_rate < 0 ? -sim->rabbits[i].survival_rate : sim->rabbits[i].survival_rate;
        if (sim->rabbits[i].age >= 120)
        {
            sim->rabbits[i].survival_rate -= 10 * ((sim->rabbits[i].age - 120) / 12);
        }
    }
    else if (sim->rabbits[i].mature && sim->rabbits[i].age == sim->rabbits[i].maturity_age)
    {
        sim->rabbits[i].survival_rate = 60;
    }
}

int generate_litters_per_year()
{
    double rand_val = genrand_real1();
    if (rand_val < 0.05) return 3;
    else if (rand_val < 0.15) return 4;
    else if (rand_val < 0.40) return 5;
    else if (rand_val < 0.70) return 6;
    else if (rand_val < 0.90) return 7;
    else if (rand_val < 0.97) return 8;
    else return 9;
}

void update_litters_per_year(s_simulation_instance *sim, size_t i)
{
    if (sim->rabbits[i].sex == 'F' && sim->rabbits[i].mature && (sim->rabbits[i].age - sim->rabbits[i].maturity_age) % 12 == 0)
    {
        sim->rabbits[i].nb_litters_y = generate_litters_per_year();
    }
}

int can_be_pregnant_this_month(s_simulation_instance *sim, size_t i)
{
    int remaining_months = 12 - (sim->rabbits[i].age - sim->rabbits[i].maturity_age) % 12 + 1;
    int remaining_litters = sim->rabbits[i].nb_litters_y - sim->rabbits[i].nb_litters;

    if (remaining_litters <= 0) return 0;

    float base_prob = (float)remaining_litters / remaining_months;
    return (genrand_real1() <= base_prob) ? 1 : 0;
}

int give_birth(s_simulation_instance *sim, size_t i)
{
    int nb_new_born = 0;
    if (sim->rabbits[i].pregnant)
    {
        nb_new_born += 3 + ((int)(genrand_real1() * 100) % 25);
        sim->rabbits[i].pregnant = 0;
        sim->rabbits[i].nb_litters += 1;
    }
    return nb_new_born;
}

void check_pregnancy(s_simulation_instance *sim, size_t i)
{
    if (sim->rabbits[i].sex == 'F' && can_be_pregnant_this_month(sim, i))
    {
        sim->rabbits[i].pregnant = 1;
    }
}

void create_new_generation(s_simulation_instance *sim, int nb_new_born)
{
    for (int j = 0; j < nb_new_born; ++j)
    {
        add_rabbit(sim);
    }
}

void update_rabbits(s_simulation_instance *sim)
{
    int nb_new_born = 0;
    for (size_t i = 0; i < sim->rabbit_count; ++i)
    {
        s_rabbit *r = &sim->rabbits[i];
        if (r->status == 0) continue;

        r->age += 1;

        update_maturity(sim, i);
        update_litters_per_year(sim, i);
        update_survival_rate(sim, i);
        check_survival(sim, i);
        nb_new_born += give_birth(sim, i);
        check_pregnancy(sim, i);
    }
    create_new_generation(sim, nb_new_born);
}

float* simulate(s_simulation_instance *sim, int months, int initial_population_nb)
{
    if (initial_population_nb == 2)
    {
        init_2_super_rabbits(sim);
    }
    else
    {
        init_starting_population(sim, initial_population_nb);
    }

    for (int current_month = 0; current_month < months; ++current_month)
    {
        if (sim->free_count == sim->rabbit_count)
        {
            break;
        }
        update_rabbits(sim);
    }
    
    int alive_rabbits_count = sim->rabbit_count - sim->free_count;
    return stock_data(2, (double)sim->dead_rabbit_count, (double)alive_rabbits_count);
}

float* stock_data(int count, ...) {
    float* arr = malloc(count * sizeof(float));
    if (!arr) return NULL;

    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++)
        arr[i] = (float)va_arg(args, double);
    va_end(args);

    return arr;
}

void multi_simulate(int months, int initial_population_nb, int nb_simulation) {
    float total_population = 0, total_dead_rabbits = 0;

    #pragma omp parallel for reduction(+:total_population, total_dead_rabbits)
    for (int i = 0; i < nb_simulation; i++) {

        // Each thread gets its own private simulation instance on its stack.
        s_simulation_instance sim_instance = {0};

        // CRITICAL: Each thread must seed its own random number generator.
        init_genrand((unsigned long)omp_get_thread_num());

        float* tab = simulate(&sim_instance, months, initial_population_nb);

        total_dead_rabbits += tab[0];
        total_population += tab[1];

        free(tab);

        // Clean up the memory allocated by this specific simulation run.
        reset_population(&sim_instance);
    }

    float avg_alive_rabbits = total_population / (float)nb_simulation;
    float avg_dead_rabbits = total_dead_rabbits / (float)nb_simulation;

    printf("\n\n--> the average <--\n    input:\n       start number: %d\n       months : %d\n       number of simulations : %d\n    results:\n       average dead population : %.2f\n"
        "       average alive rabbits : %.2f\n",
        initial_population_nb, months, nb_simulation, avg_dead_rabbits, avg_alive_rabbits);
}

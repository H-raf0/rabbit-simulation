#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <omp.h>

#include "rabbitsim.h" // Includes pcg_basic.h via our header

/**
 * A helper function to generate a random double between [0, 1).
 */
double genrand_real(pcg32_random_t *rng)
{
    return (double)pcg32_random_r(rng) / (double)UINT32_MAX;
}

int fibonacci(int n)
{
    if (n == 0 || n == 1)
        return 2;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int ensure_capacity(s_simulation_instance *sim)
{
    if (sim->rabbit_count < sim->rabbit_capacity)
        return 1;
    size_t new_capacity = (sim->rabbit_capacity == 0) ? INIT_RABIT_CAPACITY : sim->rabbit_capacity * 2;

    s_rabbit *temp_rabbits = realloc(sim->rabbits, sizeof(s_rabbit) * new_capacity);
    if (!temp_rabbits)
        return 0;
    sim->rabbits = temp_rabbits;

    int *temp_indices = realloc(sim->free_indices, sizeof(int) * new_capacity);
    if (!temp_indices)
        return 0;
    sim->free_indices = temp_indices;

    sim->rabbit_capacity = new_capacity;
    return 1;
}

char generate_sex(pcg32_random_t *rng)
{
    return (genrand_real(rng) < 0.5) ? 'M' : 'F';
}

void add_rabbit(s_simulation_instance *sim, pcg32_random_t *rng)
{
    if (!ensure_capacity(sim))
        return;

    s_rabbit *r;
    if (sim->free_count > 0)
    {
        r = &sim->rabbits[sim->free_indices[--sim->free_count]];
    }
    else
    {
        r = &sim->rabbits[sim->rabbit_count++];
    }

    r->sex = generate_sex(rng);
    r->status = 1;
    r->age = 0;
    r->mature = 0;
    r->maturity_age = 0;
    r->pregnant = 0;
    r->nb_litters_y = 0;
    r->nb_litters = 0;
    r->survival_rate = INIT_SRV_RATE;
}

void init_2_super_rabbits(s_simulation_instance *sim, pcg32_random_t *rng)
{
    add_rabbit(sim, rng);
    add_rabbit(sim, rng);
    sim->rabbits[0].sex = 'M';
    sim->rabbits[0].mature = 1;
    sim->rabbits[0].survival_rate = 100;
    sim->rabbits[1].sex = 'F';
    sim->rabbits[1].mature = 1;
    sim->rabbits[1].survival_rate = 100;
}

void init_starting_population(s_simulation_instance *sim, int nb_rabbits, pcg32_random_t *rng)
{
    for (int i = 0; i < nb_rabbits; ++i)
    {
        add_rabbit(sim, rng);
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

int check_maturity(int age, pcg32_random_t *rng)
{
    float chance = (float)age / 8.0f;
    return (genrand_real(rng) <= chance);
}

void update_maturity(s_simulation_instance *sim, size_t i, pcg32_random_t *rng)
{
    if (sim->rabbits[i].mature)
        return;
    if (sim->rabbits[i].age >= 5)
    {
        if (check_maturity(sim->rabbits[i].age, rng))
        {
            sim->rabbits[i].mature = 1;
            sim->rabbits[i].maturity_age = sim->rabbits[i].age;
        }
    }
}

int check_survival_rate(int survival_rate, pcg32_random_t *rng)
{
    if (survival_rate < 0)
        return 1;
    return (genrand_real(rng) * 100.0 <= (double)survival_rate);
}

void kill_rabbit(s_simulation_instance *sim, size_t i)
{
    sim->rabbits[i].status = 0;
    sim->free_indices[sim->free_count++] = i;
    sim->dead_rabbit_count++;
}

void check_survival(s_simulation_instance *sim, size_t i, pcg32_random_t *rng)
{
    if (sim->rabbits[i].status == 1)
    {
        if (!check_survival_rate(sim->rabbits[i].survival_rate, rng))
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

int generate_litters_per_year(pcg32_random_t *rng)
{
    double rand_val = genrand_real(rng);
    if (rand_val < 0.05)
        return 3;
    if (rand_val < 0.15)
        return 4;
    if (rand_val < 0.40)
        return 5;
    if (rand_val < 0.70)
        return 6;
    if (rand_val < 0.90)
        return 7;
    if (rand_val < 0.97)
        return 8;
    return 9;
}

void update_litters_per_year(s_simulation_instance *sim, size_t i, pcg32_random_t *rng)
{
    if (sim->rabbits[i].sex == 'F' && sim->rabbits[i].mature && (sim->rabbits[i].age - sim->rabbits[i].maturity_age) % 12 == 0)
    {
        sim->rabbits[i].nb_litters_y = generate_litters_per_year(rng);
    }
}

int can_be_pregnant_this_month(s_simulation_instance *sim, size_t i, pcg32_random_t *rng)
{
    int remaining_months = 12 - (sim->rabbits[i].age - sim->rabbits[i].maturity_age) % 12 + 1;
    int remaining_litters = sim->rabbits[i].nb_litters_y - sim->rabbits[i].nb_litters;
    if (remaining_litters <= 0)
        return 0;
    float base_prob = (float)remaining_litters / remaining_months;
    return (genrand_real(rng) <= base_prob);
}

int give_birth(s_simulation_instance *sim, size_t i, pcg32_random_t *rng)
{
    if (sim->rabbits[i].pregnant)
    {
        sim->rabbits[i].pregnant = 0;
        sim->rabbits[i].nb_litters += 1;
        // Generate a number from 0 to 3
        uint32_t extra_kittens = pcg32_boundedrand_r(rng, 4);
        return 3 + extra_kittens; // Returns 3, 4, 5, or 6
    }
    return 0;
}

void check_pregnancy(s_simulation_instance *sim, size_t i, pcg32_random_t *rng)
{
    if (sim->rabbits[i].sex == 'F' && can_be_pregnant_this_month(sim, i, rng))
    {
        sim->rabbits[i].pregnant = 1;
    }
}

void create_new_generation(s_simulation_instance *sim, int nb_new_born, pcg32_random_t *rng)
{
    for (int j = 0; j < nb_new_born; ++j)
    {
        add_rabbit(sim, rng);
    }
}

void update_rabbits(s_simulation_instance *sim, pcg32_random_t *rng)
{
    int nb_new_born = 0;
    for (size_t i = 0; i < sim->rabbit_count; ++i)
    {
        if (sim->rabbits[i].status == 0)
            continue;
        sim->rabbits[i].age += 1;
        update_maturity(sim, i, rng);
        update_litters_per_year(sim, i, rng);
        update_survival_rate(sim, i);
        check_survival(sim, i, rng);
        nb_new_born += give_birth(sim, i, rng);
        check_pregnancy(sim, i, rng);
    }
    create_new_generation(sim, nb_new_born, rng);
}

float *simulate(s_simulation_instance *sim, int months, int initial_population_nb, pcg32_random_t *rng)
{
    if (initial_population_nb == 2)
    {
        init_2_super_rabbits(sim, rng);
    }
    else
    {
        init_starting_population(sim, initial_population_nb, rng);
    }

    for (int m = 0; m < months; ++m)
    {
        if (sim->free_count == sim->rabbit_count)
            break;
        update_rabbits(sim, rng);
    }

    int alive_rabbits_count = sim->rabbit_count - sim->free_count;
    return stock_data(2, (double)sim->dead_rabbit_count, (double)alive_rabbits_count);
}

float *stock_data(int count, ...)
{
    float *arr = malloc(count * sizeof(float));
    if (!arr)
        return NULL;
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++)
    {
        arr[i] = (float)va_arg(args, double);
    }
    va_end(args);
    return arr;
}

void multi_simulate(int months, int initial_population_nb, int nb_simulation, uint64_t base_seed)
{
    float total_population = 0, total_dead_rabbits = 0;
    int sims_done = 0;

    #pragma omp parallel for reduction(+ : total_population, total_dead_rabbits)
    for (int i = 0; i < nb_simulation; i++)
    {
        s_simulation_instance sim_instance = {0};
        pcg32_random_t rng;
        pcg32_srandom_r(&rng, base_seed, (uint64_t)omp_get_thread_num());

        float *tab = simulate(&sim_instance, months, initial_population_nb, &rng);

        total_dead_rabbits += tab[0];
        total_population += tab[1];

        free(tab);
        reset_population(&sim_instance);

        // PROGRESS BAR

        // Atomically (safely) increment the shared counter.
        #pragma omp atomic update
        sims_done++;

        // Get the ID of the current thread.
        int thread_id = omp_get_thread_num();

        // Check if this is the master thread (thread 0).
        // Only this one thread will ever enter the block and print.
        // This avoids the nesting rule violation and is the standard way to do this.
        if (PRINT_OUTPUT)
        {
            if (thread_id == 0)
            {
                float progress = (float)sims_done * 100.0f / nb_simulation;

                LOG_PRINT("\r    Simulations Complete: %3d / %3d (%3.0f%%)", sims_done, nb_simulation, progress);

                fflush(stdout);
            }
        }
    }

    // After the loop, print a newline to move off the progress bar line.
    LOG_PRINT("\n");

    float avg_alive_rabbits = total_population / (float)nb_simulation;
    float avg_dead_rabbits = total_dead_rabbits / (float)nb_simulation;

    printf("\n\n--> the average <--\n    input:\n       start number: %d\n       months : %d\n       number of simulations : %d\n    results:\n       average dead population : %.2f\n"
           "       average alive rabbits : %.2f\n",
           initial_population_nb, months, nb_simulation, avg_dead_rabbits, avg_alive_rabbits);
}
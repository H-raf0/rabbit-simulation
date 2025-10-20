#include <stdio.h>
#include <stdlib.h>

#include "rabbitsim.h"
#include "mt19937ar.h"

s_rabbit *rabbits = NULL;
size_t rabbit_count = 0;
size_t dead_rabbit_count = 0;
int free_indices[MAX_RABBITS];
size_t free_count = 0;

/*
|------------------------------------------------------------------------------|
|fibonacci                                                                     |
|use : simulate the production of rabbits using fibonacci method.              |
|                                                                              |
|input : n, number of months to simulate.                                      |
|                                                                              |
|output : the number of rabbits after n months.                                |
|------------------------------------------------------------------------------|
*/
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

/*
|------------------------------------------------------------------------------|
|s_rabbit                                                                      |
|use : a structure that represent one rabbit.                                  |
|------------------------------------------------------------------------------|
*/

void init_population()
{
    rabbits = malloc(sizeof(s_rabbit) * MAX_RABBITS);
    if (!rabbits)
    {
        fprintf(stderr, "Allocation failed\n");
        exit(EXIT_FAILURE);
    }
}

void init_starting_population(int nb_rabbits)
{
    if (rabbits == NULL)
    {
        init_population();
    }
    if (nb_rabbits == 2)
    {
        add_rabbit();
        add_rabbit();
        rabbits[0].sex = 'M';
        rabbits[0].mature = 1;
        rabbits[0].survival_rate = 100;
        rabbits[1].sex = 'F';
        rabbits[1].mature = 1;
        rabbits[1].survival_rate = 100;
        return;
    }
    for (int i = 0; i < nb_rabbits; ++i)
    {
        add_rabbit();
    }
}

void reset_population()
{
    free(rabbits);
    rabbits = NULL;
    rabbit_count = 0;
    free_count = 0;
}

void add_rabbit()
{
    if (rabbit_count >= MAX_RABBITS)
    {
        printf("\n\nMaximum rabbit population reached!\n\n");
        return;
    }
    s_rabbit *r;

    if (free_count > 0)
    {
        size_t index = free_indices[--free_count];
        r = &rabbits[index];
    }
    else
    {
        r = &rabbits[rabbit_count++];
    }

    r->sex = generate_sex();
    r->status = 1;
    r->age = 0;
    r->mature = 0;
    r->maturity_age = 0;
    r->pregnant = 0;
    r->nb_litters_y = 0;
    r->nb_litters = 0;
    r->survival_rate = INIT_SRV_RATE; // initial survival rate
}

char generate_sex()
{
    return (genrand_real1() * 100 <= 50) ? 'M' : 'F';
}

int check_maturity(int age)
{
    float chance = (float)age / 8.0f; // linear increase from 0% at 0 months to 100% at 8 months
    return (genrand_real1() <= chance) ? 1 : 0;
}

void update_maturity(size_t i)
{
    if (rabbits[i].mature)
        return; // already mature

    if (rabbits[i].age >= 5 && rabbits[i].mature == 0)
    { // check maturity
        int mature = check_maturity(rabbits[i].age);
        if (mature)
        {
            rabbits[i].mature = 1;
            rabbits[i].maturity_age = rabbits[i].age;
        }
    }
}

void check_survival(size_t i)
{
    if (rabbits[i].status == 1)
    { // check survival
        if (!check_survival_rate(rabbits[i].survival_rate))
        {
            kill_rabbit(i);
            return;
        }
        else
        {
            rabbits[i].survival_rate *= -1; // invert survival rate to indicate survival this check
        }
    }
}

int check_survival_rate(int survival_rate)
{
    if (survival_rate < 0)
        return 1;
    return (genrand_real1() * 100 <= survival_rate) ? 1 : 0;
}

// checking every year because rabbits have a higher chance of dying as they age
void update_survival_rate(size_t i)
{
    if (rabbits[i].age % 12 == 0)
    {
        rabbits[i].survival_rate = rabbits[i].survival_rate < 0 ? -rabbits[i].survival_rate : rabbits[i].survival_rate; // reset if survived last check

        if (rabbits[i].age >= 120)
        { // every year after 10 years
            rabbits[i].survival_rate -= 10 * ((rabbits[i].age - 120) / 12);
        }
    }
    else if (rabbits[i].mature && rabbits[i].age == rabbits[i].maturity_age)
    { // just matured
        rabbits[i].survival_rate = 60;
    }
}

void kill_rabbit(size_t i)
{
    rabbits[i].status = 0;
    free_indices[free_count++] = i;
    dead_rabbit_count++;
}

int generate_litters_per_year()
{
    double rand_val = genrand_real1(); // Generate a random number between 0 and 1

    if (rand_val < 0.05)
        return 3; // 5% chance
    else if (rand_val < 0.15)
        return 4; // 10% chance
    else if (rand_val < 0.40)
        return 5; // 25% chance
    else if (rand_val < 0.70)
        return 6; // 30% chance
    else if (rand_val < 0.90)
        return 7; // 20% chance
    else if (rand_val < 0.97)
        return 8; // 7% chance
    else
        return 9; // 3% chance
}

void update_litters_per_year(size_t i)
{
    if (rabbits[i].sex == 'F' && rabbits[i].mature && (rabbits[i].age - rabbits[i].maturity_age) % 12 == 0)
    {
        rabbits[i].nb_litters_y = generate_litters_per_year();
    }
}

int can_be_pregnant_this_month(size_t i)
{
    int remaining_months = 12 - (rabbits[i].age - rabbits[i].maturity_age) % 12 + 1;
    int remaining_litters = rabbits[i].nb_litters_y - rabbits[i].nb_litters;

    if (remaining_litters <= 0)
        return 0; // already done for this year

    // Compute probability to give birth this month
    float base_prob = (float)remaining_litters / remaining_months;

    /*
    // Add ±20% randomness
    float noise = ((genrand_int31() % 41) - 20) / 100.0f; // [-0.2, +0.2]
    float p = base_prob + base_prob * noise;
    if (p > 1.0f) p = 1.0f;
    if (p < 0.0f) p = 0.0f;
    */

    if (genrand_real1() <= base_prob)
    {
        return 1;
    }
    return 0;
}

int give_birth(size_t i)
{
    int nb_new_born = 0;
    if (rabbits[i].pregnant)
    {
        int nb_kittens = 3 + ((int)(genrand_real1()*100) % 25); // 3 to 6 kittens // not that good
        nb_new_born += nb_kittens;
        rabbits[i].pregnant = 0;    // reset pregnancy
        rabbits[i].nb_litters += 1; // increment litters count
    }
    return nb_new_born;
}

void check_pregnancy(size_t i)
{
    if (rabbits[i].sex == 'F' && can_be_pregnant_this_month(i))
    {
        rabbits[i].pregnant = 1;
    }
}

void create_new_generation(int nb_new_born)
{
    for (int j = 0; j < nb_new_born; ++j)
    {
        add_rabbit();
    }
}

void update_rabbits()
{ // update every month
    int nb_new_born = 0;

    for (size_t i = 0; i < rabbit_count; ++i)
    {
        s_rabbit *r = &rabbits[i];
        if (r->status == 0)
            continue; // dead rabbit

        r->age += 1; // age increment

        update_maturity(i);
        update_litters_per_year(i);
        update_survival_rate(i); 
        check_survival(i);
        nb_new_born += give_birth(i);
        check_pregnancy(i);
        //printf(" rabbit %zu: age %d, survival_rate %d%%\n", i, r->age, r->survival_rate < 0 ? -r->survival_rate : r->survival_rate);
    }
    create_new_generation(nb_new_born);
    // printf("\rSimulating month %3d : 100.00 %%", current_month + 1);
}

void simulate(int months, int initial_population_nb)
{
    printf("Initializing starting population with %d rabbits...\n", initial_population_nb);

    init_population();
    init_starting_population(initial_population_nb); // Start with initial_population_nb rabbits

    printf("Starting population initialized. Total rabbits: %zu\n", rabbit_count);

    for (int current_month = 0; current_month < months; ++current_month)
    {
        if (free_count == rabbit_count)
        {
            printf("\rall population is dead at the month %d / %d \n", current_month + 1, months);
            break;
        }
        update_rabbits();
        printf("\rSimulating month %3d / %3d", current_month + 1, months);
        // clear_screen();
    }
    
    printf("\nSimulation finished. dead rabbits: %zu total rabbits alive: %zu\n", dead_rabbit_count, rabbit_count - free_count);
    reset_population();
}

void clear_screen()
{
    printf("\033[2J"); // Clear entire screen
    printf("\033[H");  // Move cursor to top-left corner
}
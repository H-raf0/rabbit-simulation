
#include "rabbitsim.h" 





/**
 * @brief Generates a random double-precision floating-point number between 0 (inclusive) and 1 (exclusive).
 * @param rng A pointer to the PCG random number generator state.
 * @return A double representing a random number in [0, 1).
 */
double genrand_real(pcg32_random_t *rng)
{
    return (double)pcg32_random_r(rng) / (double)UINT32_MAX;
}

/**
 * @brief Computes the nth Fibonacci number, with a base case returning 2 for n=0 or n=1.
 * @param n The index of the Fibonacci number to compute.
 * @return The nth Fibonacci number.
 */
int fibonacci(int n)
{
    if (n == 0 || n == 1)
        return 2;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

/**
 * @brief Ensures that the simulation instance has enough capacity to add more rabbits.
 *        If not, it reallocates memory for the rabbits and free_indices arrays to double the current capacity.
 * @param sim A pointer to the s_simulation_instance.
 * @return 1 if the capacity is sufficient or successfully increased, 0 otherwise.
 */
// TO DO: find a way to reallocate memory following better rules for the free_indices instead of doubling it
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

/**
 * @brief Randomly generates a sex (0 for female, 1 for male) for a rabbit.
 * @param rng A pointer to the PCG random number generator state.
 * @return An integer representing the sex (0 or 1).
 */
int generate_sex(pcg32_random_t *rng)
{
    return (genrand_real(rng) < 0.5) ? 0 : 1;
}

/**
 * @brief Generates a random age for a rabbit, between 10 and 19 months (inclusive).
 * @param rng A pointer to the PCG random number generator state.
 * @return An integer representing the random age.
 */
int generate_random_age(pcg32_random_t *rng){
    return (int)(genrand_real(rng)*10) + 10;
}

/**
 * @brief Adds a new rabbit to the simulation instance.
 *        If there are free slots (dead rabbits), it reuses one; otherwise, it appends to the end.
 * @param sim A pointer to the s_simulation_instance.
 * @param rng A pointer to the PCG random number generator state.
 * @param is_mature An integer indicating if the rabbit is mature (1) or not (0).
 * @param init_srv_rate The initial survival rate for the rabbit.
 * @param age The initial age of the rabbit.
 * @return void
 */
void add_rabbit(s_simulation_instance *sim, pcg32_random_t *rng, int is_mature, float init_srv_rate, int age)
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
    r->age = age;
    r->mature = is_mature;
    r->maturity_age = 0;
    r->pregnant = 0;
    r->nb_litters_y = 0;
    r->nb_litters = 0;
    r->survival_rate = init_srv_rate;
    r->survival_check_flag = 0;
}

/**
 * @brief Initializes the simulation with two "super" rabbits (one male, one female), both mature with a high survival rate and age 9.
 * @param sim A pointer to the s_simulation_instance.
 * @param rng A pointer to the PCG random number generator state.
 * @return void
 */
void init_2_super_rabbits(s_simulation_instance *sim, pcg32_random_t *rng)
{
    add_rabbit(sim, rng, 1, 100, 9);
    add_rabbit(sim, rng, 1, 100, 9);
    sim->rabbits[0].sex = 0; // Female
    sim->rabbits[1].sex = 1; // Male
}

/**
 * @brief Initializes the simulation with a specified number of adult rabbits, each with a random age.
 * @param sim A pointer to the s_simulation_instance.
 * @param nb_rabbits The number of rabbits to add to the initial population.
 * @param rng A pointer to the PCG random number generator state.
 * @return void
 */
void init_starting_population(s_simulation_instance *sim, int nb_rabbits, pcg32_random_t *rng)
{
    for (int i = 0; i < nb_rabbits; ++i)
    {
        add_rabbit(sim, rng, 1, ADULT_SRV_RATE, generate_random_age(rng));
    }
}

/**
 * @brief Frees all dynamically allocated memory for rabbits and free_indices, resetting the simulation instance to an empty state.
 * @param sim A pointer to the s_simulation_instance.
 * @return void
 */
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

/**
 * @brief Determines if a rabbit of a given age becomes mature based on a probabilistic chance.
 *        The chance increases with age.
 * @param age The current age of the rabbit.
 * @param rng A pointer to the PCG random number generator state.
 * @return 1 if the rabbit becomes mature, 0 otherwise.
 */
int check_maturity(int age, pcg32_random_t *rng)
{
    float chance = (float)age / 8.0f;
    return (genrand_real(rng) <= chance);
}

/**
 * @brief Updates the maturity status of a rabbit.
 *        If the rabbit is not yet mature and its age is 5 months or more, it checks if it becomes mature.
 * @param sim A pointer to the s_simulation_instance.
 * @param i The index of the rabbit in the simulation's rabbits array.
 * @param rng A pointer to the PCG random number generator state.
 * @return void
 */
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

/**
 * @brief Checks if a rabbit survives based on its survival rate.
 *        If the rabbit has already been checked for survival this month (survival_check_flag is 1), it automatically survives.
 * @param sim A pointer to the s_simulation_instance.
 * @param i The index of the rabbit in the simulation's rabbits array.
 * @param rng A pointer to the PCG random number generator state.
 * @return 1 if the rabbit survives, 0 otherwise.
 */
int check_survival_rate(s_simulation_instance *sim, size_t i, pcg32_random_t *rng)
{
    return sim->rabbits[i].survival_check_flag ? 1 : (genrand_real(rng) * 100.0 <= sim->rabbits[i].survival_rate);
}

/**
 * @brief Sets a rabbit's status to dead and adds its index to the list of free indices.
 * @param sim A pointer to the s_simulation_instance.
 * @param i The index of the rabbit to kill.
 * @return void
 */
void kill_rabbit(s_simulation_instance *sim, size_t i)
{
    sim->rabbits[i].status = 0;
    sim->free_indices[sim->free_count++] = i;
    sim->dead_rabbit_count++;
}

/**
 * @brief Checks if a living rabbit survives the current month. If not, it kills the rabbit.
 *        If it survives, a flag is set to prevent duplicate checks within the same month.
 * @param sim A pointer to the s_simulation_instance.
 * @param i The index of the rabbit to check.
 * @param rng A pointer to the PCG random number generator state.
 * @return void
 */
void check_survival(s_simulation_instance *sim, size_t i, pcg32_random_t *rng)
{
    if (sim->rabbits[i].status == 1)
    {
        if (!check_survival_rate(sim, i, rng))
        {
            kill_rabbit(sim, i);
        }
        else
        {
            sim->rabbits[i].survival_check_flag = 1;
        }
    }
}

/**
 * @brief Updates the survival rate of a rabbit based on its age and maturity.
 *        Resets the survival check flag monthly.
 *        Decreases survival rate for very old rabbits (120 months or older) and sets to adult rate upon reaching maturity.
 * @param sim A pointer to the s_simulation_instance.
 * @param i The index of the rabbit to update.
 * @return void
 */
void update_survival_rate(s_simulation_instance *sim, size_t i)
{
    // monthly 
    sim->rabbits[i].survival_check_flag = 0;

    if (sim->rabbits[i].age % 12 == 0)
    {
        // in case i want the change to survive check to be yearly instead of monthly
        //sim->rabbits[i].survival_check_flag = 0;
        if (sim->rabbits[i].age >= 120)
        {
            sim->rabbits[i].survival_rate -= 10 * ((sim->rabbits[i].age - 120) / 12);
        }
    }
    else if (sim->rabbits[i].mature && sim->rabbits[i].age == sim->rabbits[i].maturity_age)
    {
        sim->rabbits[i].survival_rate = ADULT_SRV_RATE;
    }
}

/**
 * @brief Randomly determines the number of litters a female rabbit can have in a year based on predefined probabilities.
 * @param rng A pointer to the PCG random number generator state.
 * @return An integer representing the number of litters per year.
 */
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

/**
 * @brief Updates the number of litters a female rabbit can have per year.
 *        This occurs annually for mature female rabbits after their maturity age.
 * @param sim A pointer to the s_simulation_instance.
 * @param i The index of the rabbit to update.
 * @param rng A pointer to the PCG random number generator state.
 * @return void
 */
void update_litters_per_year(s_simulation_instance *sim, size_t i, pcg32_random_t *rng)
{
    if (sim->rabbits[i].sex == 1 && sim->rabbits[i].mature && (sim->rabbits[i].age - sim->rabbits[i].maturity_age) % 12 == 0)
    {
        sim->rabbits[i].nb_litters_y = generate_litters_per_year(rng);
    }
}

/**
 * @brief Determines if a female rabbit can become pregnant in the current month based on its remaining litters for the year and remaining months.
 * @param sim A pointer to the s_simulation_instance.
 * @param i The index of the rabbit to check.
 * @param rng A pointer to the PCG random number generator state.
 * @return 1 if the rabbit can become pregnant, 0 otherwise.
 */
int can_be_pregnant_this_month(s_simulation_instance *sim, size_t i, pcg32_random_t *rng)
{
    int remaining_months = 12 - (sim->rabbits[i].age - sim->rabbits[i].maturity_age) % 12;
    int remaining_litters = sim->rabbits[i].nb_litters_y - sim->rabbits[i].nb_litters;
    if (remaining_litters <= 0)
        return 0;
    float base_prob = (float)remaining_litters / remaining_months;
    return (genrand_real(rng) <= base_prob);
}

/**
 * @brief Handles the birth process for a pregnant female rabbit.
 *        Resets the pregnant status and increments the litter count.
 *        Generates a random number of offspring (3 to 6).
 * @param sim A pointer to the s_simulation_instance.
 * @param i The index of the rabbit giving birth.
 * @param rng A pointer to the PCG random number generator state.
 * @return The number of new born rabbits, or 0 if the rabbit is not pregnant.
 */
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

/**
 * @brief Checks if a female rabbit becomes pregnant this month.
 * @param sim A pointer to the s_simulation_instance.
 * @param i The index of the rabbit to check.
 * @param rng A pointer to the PCG random number generator state.
 * @return void
 */
void check_pregnancy(s_simulation_instance *sim, size_t i, pcg32_random_t *rng)
{
    if (sim->rabbits[i].sex == 1 && can_be_pregnant_this_month(sim, i, rng))
    {
        sim->rabbits[i].pregnant = 1;
    }
}

/**
 * @brief Adds a specified number of new born rabbits to the simulation.
 *        New born rabbits are immature with an initial survival rate and age 0.
 * @param sim A pointer to the s_simulation_instance.
 * @param nb_new_born The number of new rabbits to create.
 * @param rng A pointer to the PCG random number generator state.
 * @return void
 */
void create_new_generation(s_simulation_instance *sim, int nb_new_born, pcg32_random_t *rng)
{
    for (int j = 0; j < nb_new_born; ++j)
    {
        add_rabbit(sim, rng, 0, INIT_SRV_RATE, 0);
    }
}

// TO DO : analyse for more statisques and data

/**
 * @brief Iterates through all rabbits in the simulation and updates their states for one month.
 *        This includes aging, checking survival, updating survival rates, checking maturity, handling births, and checking for new pregnancies.
 *        Finally, it creates new rabbits born this month.
 * @param sim A pointer to the s_simulation_instance.
 * @param rng A pointer to the PCG random number generator state.
 * @return void
 */
void update_rabbits(s_simulation_instance *sim, pcg32_random_t *rng)
{
    int nb_new_born = 0;
    for (size_t i = 0; i < sim->rabbit_count; ++i)
    {
        if (sim->rabbits[i].status == 0)
            continue;
        sim->rabbits[i].age += 1;
        check_survival(sim, i, rng);
        update_survival_rate(sim, i);
        update_maturity(sim, i, rng);
        update_litters_per_year(sim, i, rng);
        nb_new_born += give_birth(sim, i, rng);
        check_pregnancy(sim, i, rng);
    }
    create_new_generation(sim, nb_new_born, rng);
}

/**
 * @brief Runs a full simulation of the rabbit population over a specified number of months.
 *        Initializes the population, then iteratively updates rabbit states each month.
 *        Returns an array containing the total number of dead rabbits and the total number of alive rabbits at the end of the simulation.
 * @param sim A pointer to the s_simulation_instance.
 * @param months The total number of months to simulate.
 * @param initial_population_nb The initial number of rabbits. If 2, "super" rabbits are used.
 * @param rng A pointer to the PCG random number generator state.
 * @return A float array (allocated on heap) containing [dead_rabbits_count, alive_rabbits_count].
 */
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
        if (sim->free_count == sim->rabbit_count) // all dead
            break;
        update_rabbits(sim, rng);
    }

    int alive_rabbits_count = sim->rabbit_count - sim->free_count;
    return stock_data(2, (double)sim->dead_rabbit_count, (double)alive_rabbits_count);
}

/**
 * @brief Creates a float array from a variable number of double arguments.
 *        Typically used to package simulation results for return.
 * @param count The number of double arguments to convert and store.
 * @param ... A variable list of double arguments.
 * @return A pointer to a newly allocated float array containing the converted values, or NULL if allocation fails.
 */
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

/**
 * @brief Runs multiple simulations in parallel using OpenMP to calculate average population statistics.
 *        It initializes a separate simulation instance and random number generator for each simulation.
 *        Prints progress updates and final average results.
 * @param months The number of months for each simulation.
 * @param initial_population_nb The initial number of rabbits for each simulation.
 * @param nb_simulation The total number of simulations to run.
 * @param base_seed A base seed for the random number generators, combined with thread ID for uniqueness.
 * @return void
 */
void multi_simulate(int months, int initial_population_nb, int nb_simulation, uint64_t base_seed)
{
    float total_population = 0, total_dead_rabbits = 0;
    int sims_done = 0;

    #pragma omp parallel for reduction(+ : total_population, total_dead_rabbits)
    for (int i = 0; i < nb_simulation; i++)
    {
        s_simulation_instance sim_instance = {0};
        pcg32_random_t rng;
        // Seed RNG with base_seed and a unique value for each thread
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

    LOG_PRINT("\r    Simulations Complete: %3d / %3d (%3.0f%%)\n", nb_simulation, nb_simulation, 100.0f);

    float avg_alive_rabbits = total_population / (float)nb_simulation;
    float avg_dead_rabbits = total_dead_rabbits / (float)nb_simulation;

    printf("\n\n--> the average <--\n    input:\n       start number: %d\n       months : %d\n       number of simulations : %d\n    results:\n       average dead population : %.2f\n"
           "       average alive rabbits : %.2f\n",
           initial_population_nb, months, nb_simulation, avg_dead_rabbits, avg_alive_rabbits);
}
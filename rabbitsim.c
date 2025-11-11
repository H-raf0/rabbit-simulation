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
 * @param is_mature An integer indicating if the rabbit is mature (1) or not (0).
 * @param init_srv_rate The initial survival rate for the rabbit.
 * @param age The initial age of the rabbit.
 * @param sex the sex of the rabbit
 * @return void
 */
void add_rabbit(s_simulation_instance *sim, int is_mature, float init_srv_rate, int age, int sex)
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

    r->sex = sex;
    r->status = 1;
    r->age = age;
    r->mature = is_mature;
    r->maturity_age = 0;
    r->pregnant = 0;
    r->nb_litters_y = 0;
    r->nb_litters = 0;
    r->survival_rate = init_srv_rate;
    r->survival_check_flag = 0;

    sim->sex_distribution[sex]++;
}

/**
 * @brief Initializes the simulation with two "super" rabbits (one male, one female), both mature with a high survival rate and age 9.
 * @param sim A pointer to the s_simulation_instance.
 * @return void
 */
void init_2_super_rabbits(s_simulation_instance *sim)
{
    add_rabbit(sim, 1, 100, 9, 0);
    add_rabbit(sim, 1, 100, 9, 1);
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
        add_rabbit(sim, 1, ADULT_SRV_RATE, generate_random_age(rng), generate_sex(rng));
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
        add_rabbit(sim, 0, INIT_SRV_RATE, 0, generate_sex(rng));
    }
}

/**
 * @brief Iterates through all rabbits in the simulation and updates their states for one month.
 *        This includes aging, checking survival, updating survival rates, checking maturity, 
 *        handling births, and checking for new pregnancies.
 *        Finally, it creates new rabbits born this month.
 * 
 * @param sim A pointer to the s_simulation_instance.
 * @param rng A pointer to the PCG random number generator state.
 * @param births Pointer to store the number of births this month (output parameter).
 * @param deaths Pointer to store the number of deaths this month (output parameter).
 * @return void
 */
void update_rabbits(s_simulation_instance *sim, pcg32_random_t *rng, int *births, int *deaths)
{
    int nb_new_born = 0;
    size_t deaths_before = sim->dead_rabbit_count;
    
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
    
    // Calculate deaths and births for this month
    *deaths = sim->dead_rabbit_count - deaths_before;
    *births = nb_new_born;
    
    create_new_generation(sim, nb_new_born, rng);
}

/**
 * @brief Collects current statistics about the simulation population.
 *        Counts living rabbits by sex, maturity status, and pregnancy status.
 * 
 * @param sim A pointer to the s_simulation_instance.
 * @param males Pointer to store the count of male rabbits (output parameter).
 * @param females Pointer to store the count of female rabbits (output parameter).
 * @param mature Pointer to store the count of mature rabbits (output parameter).
 * @param pregnant Pointer to store the count of pregnant females (output parameter).
 * @return void
 */
void collect_population_stats(s_simulation_instance *sim, int *males, int *females, 
                              int *mature, int *pregnant)
{
    *males = 0;
    *females = 0;
    *mature = 0;
    *pregnant = 0;
    
    for (size_t i = 0; i < sim->rabbit_count; ++i)
    {
        if (sim->rabbits[i].status == 0)
            continue;
            
        if (sim->rabbits[i].sex == 1)
            (*males)++;
        else
            (*females)++;
            
        if (sim->rabbits[i].mature)
            (*mature)++;
            
        if (sim->rabbits[i].pregnant)
            (*pregnant)++;
    }
}

/**
 * @brief Runs a full simulation of the rabbit population over a specified number of months.
 *        Initializes the population, then iteratively updates rabbit states each month.
 *        Tracks comprehensive statistics including population dynamics, sex distribution,
 *        temporal patterns (peak/minimum populations), and monthly snapshots.
 * 
 * @param sim A pointer to the s_simulation_instance.
 * @param months The total number of months to simulate.
 * @param initial_population_nb The initial number of rabbits. If 2, "super" rabbits are used.
 * @param rng A pointer to the PCG random number generator state.
 * @return A s_simulation_results struct containing all collected statistics and monthly data.
 */
s_simulation_results simulate(s_simulation_instance *sim, int months, int initial_population_nb, pcg32_random_t *rng)
{
    // Initialize results structure with default values
    s_simulation_results results = {0};
    
    // Allocate memory for monthly data tracking
    results.monthly_data = malloc(sizeof(s_monthly_data) * months);
    if (!results.monthly_data)
    {
        // Memory allocation failed - return empty results
        return results;
    }
    
    // Variables to track population statistics during simulation
    int peak_population = 0;
    int peak_month = 0;
    int min_population = INT32_MAX;  // Start with maximum possible value
    int min_month = 0;
    long long population_sum = 0;
    int actual_months = 0;
    
    // Initialize starting population based on parameter
    if (initial_population_nb == 2)
    {
        init_2_super_rabbits(sim);
    }
    else
    {
        init_starting_population(sim, initial_population_nb, rng);
    }

    // Main simulation loop - iterate through each month
    for (int m = 0; m < months; ++m)
    {
        // Calculate current living population
        int current_alive = sim->rabbit_count - sim->free_count;
        
        // Collect detailed statistics for this month
        int males, females, mature, pregnant;
        collect_population_stats(sim, &males, &females, &mature, &pregnant);
        
        // Store monthly snapshot (before updating for this month)
        results.monthly_data[m].month = m;
        results.monthly_data[m].alive = current_alive;
        results.monthly_data[m].males = males;
        results.monthly_data[m].females = females;
        results.monthly_data[m].mature_rabbits = mature;
        results.monthly_data[m].pregnant_females = pregnant;
        
        // Check for extinction (all rabbits dead)
        if (sim->free_count == sim->rabbit_count)
        {
            results.monthly_data[m].deaths_this_month = 0;
            results.monthly_data[m].births_this_month = 0;
            results.extinction_month = m;
            actual_months = m;
            break;
        }
        
        // Track peak population (maximum alive at any point)
        if (current_alive > peak_population)
        {
            peak_population = current_alive;
            peak_month = m;
        }
        
        // Track minimum population (excluding initial month to see recovery/decline patterns)
        if (m > 0 && current_alive < min_population)
        {
            min_population = current_alive;
            min_month = m;
        }
        
        // Accumulate population for average calculation
        population_sum += current_alive;
        actual_months = m + 1;
        
        // Update all rabbits for this month and track births/deaths
        int births, deaths;
        update_rabbits(sim, rng, &births, &deaths);
        
        // Store births and deaths that occurred during this month's update
        results.monthly_data[m].deaths_this_month = deaths;
        results.monthly_data[m].births_this_month = births;
    }

    // Calculate final population counts
    int final_alive = sim->rabbit_count - sim->free_count;
    
    // Collect final sex distribution
    int final_males, final_females, final_mature, final_pregnant;
    collect_population_stats(sim, &final_males, &final_females, &final_mature, &final_pregnant);
    
    // Populate results structure with collected data
    results.total_dead = sim->dead_rabbit_count;
    results.final_alive = final_alive;
    results.final_males = final_males;
    results.final_females = final_females;
    results.peak_population = peak_population;
    results.peak_population_month = peak_month;
    
    // Only set minimum if we found a valid minimum (not if population started at 0)
    if (min_population != INT32_MAX && min_population < initial_population_nb)
    {
        results.min_population = min_population;
        results.min_population_month = min_month;
    }
    else
    {
        // If no minimum was found (population only grew), set to initial
        results.min_population = initial_population_nb;
        results.min_population_month = 0;
    }
    
    results.total_population_sum = population_sum;
    results.months_simulated = actual_months;
    results.monthly_data_count = actual_months;
    
    // Calculate sex percentages for final population
    if (final_alive > 0)
    {
        results.male_percentage = (float)results.final_males * 100.0f / final_alive;
        results.female_percentage = (float)results.final_females * 100.0f / final_alive;
    }
    else
    {
        // Population extinct - no meaningful percentages
        results.male_percentage = 0.0f;
        results.female_percentage = 0.0f;
    }

    return results;
}

/**
 * @brief Frees the dynamically allocated memory in a simulation results structure.
 * 
 * @param results A pointer to the s_simulation_results structure to free.
 * @return void
 */
void free_simulation_results(s_simulation_results *results)
{
    if (results->monthly_data)
    {
        free(results->monthly_data);
        results->monthly_data = NULL;
        results->monthly_data_count = 0;
    }
}

/**
 * @brief Writes a single simulation's monthly data to a CSV file.
 *        Creates a file with detailed month-by-month statistics suitable for plotting.
 * 
 * @param results Pointer to the simulation results structure.
 * @param filename The name of the output CSV file.
 * @param sim_number The simulation number (for multi-simulation runs).
 * @return void
 */
void write_simulation_to_csv(const s_simulation_results *results, const char *filename, int sim_number)
{
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        fprintf(stderr, "Error: Could not open file %s for writing\n", filename);
        return;
    }
    
    // Write CSV header
    fprintf(file, "simulation,month,alive,deaths,births,males,females,mature,pregnant\n");
    
    // Write monthly data
    for (int i = 0; i < results->monthly_data_count; ++i)
    {
        s_monthly_data *data = &results->monthly_data[i];
        fprintf(file, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                sim_number,
                data->month,
                data->alive,
                data->deaths_this_month,
                data->births_this_month,
                data->males,
                data->females,
                data->mature_rabbits,
                data->pregnant_females);
    }
    
    fclose(file);
    LOG_PRINT("    Monthly data written to: %s\n", filename);
}

/**
 * @brief Writes aggregated statistics from multiple simulations to a CSV file.
 *        Calculates average population values across all simulations for each month.
 *        Also writes a summary statistics file with overall averages.
 * 
 * @param months The number of months simulated.
 * @param initial_population The initial population size.
 * @param nb_simulations The total number of simulations run.
 * @param all_results Array of all simulation results.
 * @param filename The name of the output CSV file.
 * @return void
 */
void write_aggregated_stats_to_csv(int months, int initial_population, int nb_simulations, 
                                   const s_simulation_results *all_results, const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        fprintf(stderr, "Error: Could not open file %s for writing\n", filename);
        return;
    }
    
    // Write CSV header for monthly averages
    fprintf(file, "month,avg_alive,avg_deaths,avg_births,avg_males,avg_females,avg_mature,avg_pregnant,std_alive\n");
    
    // For each month, calculate averages across all simulations
    for (int m = 0; m < months; ++m)
    {
        long long sum_alive = 0, sum_deaths = 0, sum_births = 0;
        long long sum_males = 0, sum_females = 0, sum_mature = 0, sum_pregnant = 0;
        int valid_sims = 0;
        
        // Accumulate values from all simulations for this month
        for (int s = 0; s < nb_simulations; ++s)
        {
            if (m < all_results[s].monthly_data_count)
            {
                sum_alive += all_results[s].monthly_data[m].alive;
                sum_deaths += all_results[s].monthly_data[m].deaths_this_month;
                sum_births += all_results[s].monthly_data[m].births_this_month;
                sum_males += all_results[s].monthly_data[m].males;
                sum_females += all_results[s].monthly_data[m].females;
                sum_mature += all_results[s].monthly_data[m].mature_rabbits;
                sum_pregnant += all_results[s].monthly_data[m].pregnant_females;
                valid_sims++;
            }
        }
        
        if (valid_sims == 0)
            break;  // All simulations extinct before this month
        
        // Calculate averages
        float avg_alive = (float)sum_alive / valid_sims;
        float avg_deaths = (float)sum_deaths / valid_sims;
        float avg_births = (float)sum_births / valid_sims;
        float avg_males = (float)sum_males / valid_sims;
        float avg_females = (float)sum_females / valid_sims;
        float avg_mature = (float)sum_mature / valid_sims;
        float avg_pregnant = (float)sum_pregnant / valid_sims;
        
        // Calculate standard deviation for population
        float sum_sq_diff = 0.0f;
        for (int s = 0; s < nb_simulations; ++s)
        {
            if (m < all_results[s].monthly_data_count)
            {
                float diff = all_results[s].monthly_data[m].alive - avg_alive;
                sum_sq_diff += diff * diff;
            }
        }
        float std_alive = sqrtf(sum_sq_diff / valid_sims);
        
        // Write monthly averages
        fprintf(file, "%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                m, avg_alive, avg_deaths, avg_births, avg_males, avg_females, 
                avg_mature, avg_pregnant, std_alive);
    }
    
    fclose(file);
    LOG_PRINT("    Aggregated monthly data written to: %s\n", filename);
    
    // Write summary statistics file
    char summary_filename[256];
    snprintf(summary_filename, sizeof(summary_filename), "summary_stats.csv");
    
    FILE *summary = fopen(summary_filename, "w");
    if (!summary)
    {
        fprintf(stderr, "Error: Could not open file %s for writing\n", summary_filename);
        return;
    }
    
    // Calculate summary statistics
    long long total_final_alive = 0, total_final_dead = 0;
    long long total_peak = 0, total_min = 0;
    int total_extinctions = 0;
    
    for (int s = 0; s < nb_simulations; ++s)
    {
        total_final_alive += all_results[s].final_alive;
        total_final_dead += all_results[s].total_dead;
        total_peak += all_results[s].peak_population;
        total_min += all_results[s].min_population;
        if (all_results[s].extinction_month > 0)
            total_extinctions++;
    }
    
    // Write summary header and data
    fprintf(summary, "metric,value\n");
    fprintf(summary, "initial_population,%d\n", initial_population);
    fprintf(summary, "months_simulated,%d\n", months);
    fprintf(summary, "number_of_simulations,%d\n", nb_simulations);
    fprintf(summary, "avg_final_alive,%.2f\n", (float)total_final_alive / nb_simulations);
    fprintf(summary, "avg_total_deaths,%.2f\n", (float)total_final_dead / nb_simulations);
    fprintf(summary, "avg_peak_population,%.2f\n", (float)total_peak / nb_simulations);
    fprintf(summary, "avg_min_population,%.2f\n", (float)total_min / nb_simulations);
    fprintf(summary, "extinction_count,%d\n", total_extinctions);
    fprintf(summary, "extinction_rate,%.2f\n", (float)total_extinctions * 100.0f / nb_simulations);
    
    fclose(summary);
    LOG_PRINT("    Summary statistics written to: %s\n", summary_filename);
}

/**
 * @brief Runs multiple simulations in parallel using OpenMP to calculate average population statistics.
 *        Each simulation runs independently with its own random number generator.
 *        Aggregates results across all simulations, prints comprehensive statistics,
 *        and writes data to CSV files for Python visualization.
 * 
 * @param months The number of months for each simulation.
 * @param initial_population_nb The initial number of rabbits for each simulation.
 * @param nb_simulation The total number of simulations to run.
 * @param base_seed A base seed for the random number generators, combined with thread ID for uniqueness.
 * @return void
 */
void multi_simulate(int months, int initial_population_nb, int nb_simulation, uint64_t base_seed)
{
    // Allocate array to store all simulation results (for CSV export)
    s_simulation_results *all_results = malloc(sizeof(s_simulation_results) * nb_simulation);
    if (!all_results)
    {
        fprintf(stderr, "Error: Could not allocate memory for simulation results\n");
        return;
    }
    
    // Accumulators for averaging results across all simulations
    long long total_population = 0;
    long long total_dead_rabbits = 0;
    long long total_extinction_month = 0;
    long long total_males = 0;
    long long total_females = 0;
    long long total_peak_population = 0;
    long long total_peak_month = 0;
    long long total_min_population = 0;
    long long total_min_month = 0;
    long long total_avg_population_sum = 0;
    
    int nb_extinctions = 0;
    int sims_done = 0;

    // Parallel loop - each iteration runs one simulation on a separate thread
    #pragma omp parallel for reduction(+ : total_population, total_dead_rabbits, total_extinction_month, nb_extinctions, total_males, total_females, total_peak_population, total_peak_month, total_min_population, total_min_month, total_avg_population_sum)
    for (int i = 0; i < nb_simulation; i++)
    {
        // Create independent simulation instance and RNG for this thread
        s_simulation_instance sim_instance = {0};
        pcg32_random_t rng;
        
        // Seed RNG with base_seed combined with simulation index for uniqueness
        pcg32_srandom_r(&rng, base_seed, (uint64_t)i);

        // Run single simulation and get results
        s_simulation_results results = simulate(&sim_instance, months, initial_population_nb, &rng);
        
        // Store results for later CSV export
        all_results[i] = results;

        // Accumulate results for averaging
        total_dead_rabbits += results.total_dead;
        total_population += results.final_alive;
        total_males += results.final_males;
        total_females += results.final_females;
        total_peak_population += results.peak_population;
        total_peak_month += results.peak_population_month;
        total_min_population += results.min_population;
        total_min_month += results.min_population_month;
        
        // Calculate average population for this simulation
        if (results.months_simulated > 0)
        {
            total_avg_population_sum += (results.total_population_sum / results.months_simulated);
        }
        
        // Track extinction statistics
        if (results.extinction_month > 0)
        {
            total_extinction_month += results.extinction_month;
            nb_extinctions++;
        }

        // Clean up memory for this simulation (but keep monthly_data for CSV export)
        reset_population(&sim_instance);

        // Progress tracking (thread-safe increment)
        #pragma omp atomic update
        sims_done++;

        // Only master thread prints progress to avoid garbled output
        int thread_id = omp_get_thread_num();
        if (PRINT_OUTPUT && thread_id == 0)
        {
            float progress = (float)sims_done * 100.0f / nb_simulation;
            LOG_PRINT("\r    Simulations Complete: %3d / %3d (%3.0f%%)", sims_done, nb_simulation, progress);
            fflush(stdout);
        }
    }

    // Final progress update showing 100% completion
    LOG_PRINT("\r    Simulations Complete: %3d / %3d (%3.0f%%)\n", nb_simulation, nb_simulation, 100.0f);

    // Calculate averages across all simulations
    float avg_alive_rabbits = (float)total_population / nb_simulation;
    float avg_dead_rabbits = (float)total_dead_rabbits / nb_simulation;
    float avg_males = (float)total_males / nb_simulation;
    float avg_females = (float)total_females / nb_simulation;
    float avg_peak_population = (float)total_peak_population / nb_simulation;
    float avg_peak_month = (float)total_peak_month / nb_simulation;
    float avg_min_population = (float)total_min_population / nb_simulation;
    float avg_min_month = (float)total_min_month / nb_simulation;
    float avg_population_over_time = (float)total_avg_population_sum / nb_simulation;
    
    // Calculate average extinction month (only for simulations that went extinct)
    float avg_extinction_month = nb_extinctions > 0 ? (float)total_extinction_month / nb_extinctions : 0;
    float extinction_rate = (float)nb_extinctions * 100.0f / nb_simulation;

    // Calculate sex percentages from averages
    float avg_male_percentage = (avg_males + avg_females) > 0 ? 
                                (avg_males * 100.0f / (avg_males + avg_females)) : 0.0f;
    float avg_female_percentage = (avg_males + avg_females) > 0 ? 
                                  (avg_females * 100.0f / (avg_males + avg_females)) : 0.0f;

    // Format extinction string for display
    char extinction_str[64];
    if (nb_extinctions == 0)
        snprintf(extinction_str, sizeof(extinction_str), "no extinctions");
    else
        snprintf(extinction_str, sizeof(extinction_str), "%.2f (%.1f%% of simulations)", 
                 avg_extinction_month, extinction_rate);

    // Print comprehensive results to console
    printf("\n\n"
           "╔════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                    SIMULATION RESULTS SUMMARY                          ║\n");
    printf("╠════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ INPUT PARAMETERS:                                                      ║\n");
    printf("║   • Initial Population: %-10d                                     ║\n", initial_population_nb);
    printf("║   • Simulation Duration: %-10d months                             ║\n", months);
    printf("║   • Number of Simulations: %-10d                                  ║\n", nb_simulation);
    printf("╠════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ FINAL POPULATION STATISTICS:                                           ║\n");
    printf("║   • Average Living Rabbits: %-10.2f                                 ║\n", avg_alive_rabbits);
    printf("║   • Average Deaths (Total): %-10.2f                                 ║\n", avg_dead_rabbits);
    printf("║   • Average Males: %-10.2f (%.1f%%)                                  ║\n", avg_males, avg_male_percentage);
    printf("║   • Average Females: %-10.2f (%.1f%%)                                ║\n", avg_females, avg_female_percentage);
    printf("╠════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ POPULATION DYNAMICS:                                                   ║\n");
    printf("║   • Peak Population: %-10.2f (at month %.1f avg)                    ║\n", avg_peak_population, avg_peak_month);
    printf("║   • Minimum Population: %-10.2f (at month %.1f avg)               ║\n", avg_min_population, avg_min_month);
    printf("║   • Average Population Over Time: %-10.2f                           ║\n", avg_population_over_time);
    printf("╠════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ EXTINCTION ANALYSIS:                                                   ║\n");
    printf("║   • Average Extinction Month: %-35s      ║\n", extinction_str);
    printf("╚════════════════════════════════════════════════════════════════════════╝\n");
    /*
    // Write data to CSV files for Python visualization
    printf("\n📊 Writing data files for visualization...\n");
    
    // Write aggregated monthly data (averages across all simulations)
    write_aggregated_stats_to_csv(months, initial_population_nb, nb_simulation, 
                                  all_results, "monthly_averages.csv");
    
    // Optionally write individual simulation data (useful for variability analysis)
    // For large number of simulations, you might want to write only a subset
    if (nb_simulation <= 10)
    {
        for (int i = 0; i < nb_simulation; ++i)
        {
            char filename[256];
            snprintf(filename, sizeof(filename), "simulation_%d.csv", i + 1);
            write_simulation_to_csv(&all_results[i], filename, i + 1);
        }
    }
    else
    {
        // Write first 3 simulations as examples
        for (int i = 0; i < 3 && i < nb_simulation; ++i)
        {
            char filename[256];
            snprintf(filename, sizeof(filename), "simulation_%d.csv", i + 1);
            write_simulation_to_csv(&all_results[i], filename, i + 1);
        }
        printf("    (Only first 3 individual simulations written - use fewer simulations to export all)\n");
    }*/
    
    // Free all allocated memory for simulation results
    for (int i = 0; i < nb_simulation; ++i)
    {
        free_simulation_results(&all_results[i]);
    }
    free(all_results);
    
    printf("\n✅ Data export complete! Use the provided Python script to visualize results.\n");
}
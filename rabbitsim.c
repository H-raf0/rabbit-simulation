#include "rabbitsim.h" 

// Global variable for survival calculation method
survival_method_t survival_method = SURVIVAL_STATIC;





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
void add_rabbit(s_simulation_instance *sim, pcg32_random_t* rng, int is_mature, float init_srv_rate, int age, int sex)
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
    
    // Use the selected survival calculation method
    switch (survival_method)
    {
        case SURVIVAL_STATIC:
            r->survival_rate = calculate_survival_rate_static(init_srv_rate);
            break;
        case SURVIVAL_GAUSSIAN:
            r->survival_rate = calculate_survival_rate_gaussian(init_srv_rate, rng);
            break;
        case SURVIVAL_EXPONENTIAL:
            r->survival_rate = calculate_survival_rate_exponential(init_srv_rate, rng);
            break;
    }
    
    r->survival_check_flag = 0;

    sim->sex_distribution[sex]++;
}

/**
 * @brief Initializes the simulation with two "super" rabbits (one male, one female), both mature with a high survival rate and age 9.
 * @param sim A pointer to the s_simulation_instance.
 * @return void
 */
void init_2_super_rabbits(s_simulation_instance *sim, pcg32_random_t* rng)
{
    add_rabbit(sim, rng, 1, 100, 9, 0);
    add_rabbit(sim, rng, 1, 100, 9, 1);
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
        add_rabbit(sim, rng, 1, ADULT_SRV_RATE, generate_random_age(rng), generate_sex(rng));
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
    
    #if defined(ENABLE_DATA_LOGGING) && ENABLE_DATA_LOGGING != 0
    free(sim->monthly_data);
    sim->monthly_data = NULL;
    sim->monthly_data_capacity = 0;
    sim->monthly_data_count = 0;
    #endif
    
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
    
    #if defined(ENABLE_DATA_LOGGING) && ENABLE_DATA_LOGGING != 0
    sim->deaths_this_month++;
    #endif
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
 * @brief Calculates the base survival rate for a rabbit based on its age and maturity status.
 * @param sim A pointer to the s_simulation_instance.
 * @param i The index of the rabbit.
 * @return The base survival rate.
 */
float calculate_base_survival_rate(s_simulation_instance *sim, size_t i)
{
    float base_rate;
    
    if (!sim->rabbits[i].mature) {
        // Young rabbits use initial survival rate
        base_rate = INIT_SRV_RATE;
    } else {
        // Mature rabbits use adult survival rate
        base_rate = ADULT_SRV_RATE;
    }
    
    // Apply age penalty for very old rabbits (120 months and older)
    if (sim->rabbits[i].age >= 120) {
        base_rate -= 10 * ((sim->rabbits[i].age - 120) / 12);
        if (base_rate < 0.0f) base_rate = 0.0f;
    }
    
    return base_rate;
}

/**
 * @brief Updates the survival rate of a rabbit based on its age and maturity.
 *        Resets the survival check flag monthly.
 *        Applies age-based penalties and uses the selected survival calculation method.
 * @param sim A pointer to the s_simulation_instance.
 * @param i The index of the rabbit to update.
 * @param rng A pointer to the PCG random number generator state.
 * @return void
 */
void update_survival_rate(s_simulation_instance *sim, size_t i, pcg32_random_t *rng)
{
    // monthly 
    sim->rabbits[i].survival_check_flag = 0;

    // Calculate base survival rate based on age and maturity
    float base_rate = calculate_base_survival_rate(sim, i);
    
    // Apply the selected survival calculation method
    switch (survival_method)
    {
        case SURVIVAL_STATIC:
            // For static method, only update when becoming mature or yearly for old age penalty
            if (sim->rabbits[i].mature && sim->rabbits[i].age == sim->rabbits[i].maturity_age)
            {
                sim->rabbits[i].survival_rate = calculate_survival_rate_static(base_rate);
            }
            else if (sim->rabbits[i].age % 12 == 0 && sim->rabbits[i].age >= 120)
            {
                // Apply age penalty for very old rabbits
                sim->rabbits[i].survival_rate = calculate_survival_rate_static(base_rate);
            }
            break;
            
        case SURVIVAL_GAUSSIAN:
            // Apply Gaussian variation every month
            sim->rabbits[i].survival_rate = calculate_survival_rate_gaussian(base_rate, rng);
            break;
            
        case SURVIVAL_EXPONENTIAL:
            // Apply exponential variation every month
            sim->rabbits[i].survival_rate = calculate_survival_rate_exponential(base_rate, rng);
            break;
    }
}

/**
 * @brief Calculates survival rate using static (constant) method.
 * @param base_rate The base survival rate.
 * @return The calculated survival rate (same as base_rate).
 */
float calculate_survival_rate_static(float base_rate)
{
    return base_rate;
}

/**
 * @brief Calculates survival rate using Gaussian distribution.
 * @param base_rate The base survival rate (used as mean).
 * @param rng A pointer to the PCG random number generator state.
 * @return The calculated survival rate following a normal distribution.
 */
float calculate_survival_rate_gaussian(float base_rate, pcg32_random_t *rng)
{
    // Box-Muller transform for Gaussian distribution
    double u1 = genrand_real(rng);
    double u2 = genrand_real(rng);
    double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    
    // Use base_rate as mean, and 5.0 as standard deviation
    float result = (float)(base_rate + 5.0 * z0);
    
    // Clamp to reasonable bounds (0-100)
    if (result < 0.0f) result = 0.0f;
    if (result > 100.0f) result = 100.0f;
    
    return result;
}

/**
 * @brief Calculates survival rate using exponential distribution.
 * @param base_rate The base survival rate (used as rate parameter).
 * @param rng A pointer to the PCG random number generator state.
 * @return The calculated survival rate following an exponential distribution.
 */
float calculate_survival_rate_exponential(float base_rate, pcg32_random_t *rng)
{
    double u = genrand_real(rng);
    // Exponential distribution: rate = 1/base_rate, but we want higher base_rate to mean higher survival
    // So we use: survival = 100 * (1 - exp(-base_rate/100 * random_factor))
    double lambda = 1.0 / (base_rate / 10.0); // Adjust lambda based on base_rate
    double result = -log(u) / lambda;
    
    // Convert to percentage and clamp
    result = 100.0 * (1.0 - exp(-result));
    if (result < 0.0) result = 0.0;
    if (result > 100.0) result = 100.0;
    
    return (float)result;
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
    #if defined(ENABLE_DATA_LOGGING) && ENABLE_DATA_LOGGING != 0
    sim->births_this_month += nb_new_born;
    #endif
    
    for (int j = 0; j < nb_new_born; ++j)
    {
        add_rabbit(sim, rng, 0, INIT_SRV_RATE, 0, generate_sex(rng));
    }
}

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
    
    #if defined(ENABLE_DATA_LOGGING) && ENABLE_DATA_LOGGING != 0
    sim->deaths_this_month = 0;
    sim->births_this_month = 0;
    #endif
    
    for (size_t i = 0; i < sim->rabbit_count; ++i)
    {
        if (sim->rabbits[i].status == 0)
            continue;
        sim->rabbits[i].age += 1;
        check_survival(sim, i, rng);
        update_survival_rate(sim, i, rng);
        update_maturity(sim, i, rng);
        update_litters_per_year(sim, i, rng);
        nb_new_born += give_birth(sim, i, rng);
        check_pregnancy(sim, i, rng);
    }
    create_new_generation(sim, nb_new_born, rng);
}

// ===== NEW LOGGING FUNCTIONS =====

#if defined(ENABLE_DATA_LOGGING) && ENABLE_DATA_LOGGING != 0

/**
 * @brief Initializes the monthly logging system by allocating memory for monthly statistics.
 * @param sim A pointer to the s_simulation_instance.
 * @param months The maximum number of months to simulate.
 * @return void
 */
void init_monthly_logging(s_simulation_instance *sim, int months)
{
    sim->monthly_data = malloc(sizeof(s_monthly_stats) * months);
    if (sim->monthly_data)
    {
        sim->monthly_data_capacity = months;
        sim->monthly_data_count = 0;
    }
    sim->deaths_this_month = 0;
    sim->births_this_month = 0;
}

/**
 * @brief Records statistics for the current month by analyzing all living rabbits.
 * @param sim A pointer to the s_simulation_instance.
 * @param month The current month number.
 * @return void
 */
// TO DO: I already now the number of alive rabbits et the number of males and females, maybe I should just add another for age instead of going through the whole array again
void record_monthly_stats(s_simulation_instance *sim, int month, int alive_count, int males, int females)
{
    if (!sim->monthly_data || sim->monthly_data_count >= sim->monthly_data_capacity)
        return;
    
    s_monthly_stats *stats = &sim->monthly_data[sim->monthly_data_count++];
    stats->month = month;
    stats->males = males;
    stats->females = females;
    stats->mature_rabbits = 0;
    stats->pregnant_females = 0;
    stats->births_this_month = sim->births_this_month;
    stats->deaths_this_month = sim->deaths_this_month;
    
    long long age_sum = 0;
    int min_age = INT_MAX;
    int max_age = INT_MIN;
    
    for (size_t i = 0; i < sim->rabbit_count; ++i)
    {
        if (sim->rabbits[i].status == 0)
            continue;
            
        int age = sim->rabbits[i].age;
        age_sum += age;
        if (age < min_age) min_age = age;
        if (age > max_age) max_age = age;
            
        if (sim->rabbits[i].mature)
            stats->mature_rabbits++;
            
        if (sim->rabbits[i].pregnant)
            stats->pregnant_females++;
    }
    
    stats->total_alive = alive_count;
    stats->avg_age = alive_count > 0 ? (float)age_sum / alive_count : 0.0f;
    stats->min_age = (min_age == INT_MAX) ? 0 : min_age;
    stats->max_age = (max_age == INT_MIN) ? 0 : max_age;
}

/**
 * @brief Writes the monthly statistics for a single simulation to a CSV file.
 * @param sim A pointer to the s_simulation_instance.
 * @param sim_number The simulation number (for filename).
 * @param initial_population The initial population size (for filename).
 * @return void
 */
void write_simulation_log(s_simulation_instance *sim, int sim_number, int initial_population)
{
    if (!sim->monthly_data || sim->monthly_data_count == 0)
        return;
    
    char filename[256];
    snprintf(filename, sizeof(filename), "simulation_%d_pop%d.csv", sim_number, initial_population);
    
    FILE *fp = fopen(filename, "w");
    if (!fp)
    {
        LOG_PRINT("Warning: Could not create log file %s\n", filename);
        return;
    }
    
    // Write CSV header
    fprintf(fp, "Month,Total_Alive,Males,Females,Male_Percentage,Female_Percentage,Mature_Rabbits,Pregnant_Females,Births,Deaths,Avg_Age,Min_Age,Max_Age\n");
    
    // Write data for each month
    for (int i = 0; i < sim->monthly_data_count; ++i)
    {
        s_monthly_stats *s = &sim->monthly_data[i];
        float male_pct = s->total_alive > 0 ? (float)s->males * 100.0f / s->total_alive : 0.0f;
        float female_pct = s->total_alive > 0 ? (float)s->females * 100.0f / s->total_alive : 0.0f;
        
        fprintf(fp, "%d,%d,%d,%d,%.2f,%.2f,%d,%d,%d,%d,%.2f,%d,%d\n",
                s->month, s->total_alive, s->males, s->females,
                male_pct, female_pct, s->mature_rabbits, s->pregnant_females,
                s->births_this_month, s->deaths_this_month, s->avg_age, s->min_age, s->max_age);
    }
    
    fclose(fp);
    //LOG_PRINT("    Logged simulation %d data to %s\n", sim_number, filename);
}

/**
 * @brief Writes a summary CSV file containing aggregate statistics from all simulations.
 * @param months The number of months simulated.
 * @param initial_population The initial population size.
 * @param nb_simulations The total number of simulations run.
 * @param all_results Array of results from all simulations.
 * @param base_seed The base random seed used.
 * @return void
 */
void write_summary_log(int months, int initial_population, int nb_simulations,
                       s_simulation_results *all_results, uint64_t base_seed)
{
    if (!all_results)
        return;
    
    char filename[256];
    snprintf(filename, sizeof(filename), "simulation_summary_pop%d_%dsims.csv", 
             initial_population, nb_simulations);
    
    FILE *fp = fopen(filename, "w");
    if (!fp)
    {
        LOG_PRINT("Warning: Could not create summary file %s\n", filename);
        return;
    }
    
    // Write metadata as comments
    fprintf(fp, "# Rabbit Simulation Summary\n");
    fprintf(fp, "# Initial Population: %d\n", initial_population);
    fprintf(fp, "# Months Simulated: %d\n", months);
    fprintf(fp, "# Number of Simulations: %d\n", nb_simulations);
    fprintf(fp, "# Base Seed: %llu\n", (unsigned long long)base_seed);
    fprintf(fp, "#\n");
    
    // Write CSV header
    fprintf(fp, "Sim_Number,Final_Alive,Total_Dead,Final_Males,Final_Females,Male_Pct,Female_Pct,Peak_Pop,Peak_Month,Min_Pop,Min_Month,Extinction_Month,Months_Simulated\n");
    
    // Write data for each simulation
    for (int i = 0; i < nb_simulations; ++i)
    {
        s_simulation_results *r = &all_results[i];
        fprintf(fp, "%d,%d,%d,%d,%d,%.2f,%.2f,%d,%d,%d,%d,%d,%d\n",
                i + 1, r->final_alive, r->total_dead, 
                r->final_males, r->final_females,
                r->male_percentage, r->female_percentage,
                r->peak_population, r->peak_population_month,
                r->min_population, r->min_population_month,
                r->extinction_month, r->months_simulated);
    }
    
    fclose(fp);
    //LOG_PRINT("    Summary written to %s\n", filename);
}

#else
// Dummy implementations when logging is disabled
void init_monthly_logging(s_simulation_instance *sim, int months) { (void)sim; (void)months; }
void record_monthly_stats(s_simulation_instance *sim, int month, int alive_count, int males, int females) { (void)sim; (void)month; (void)alive_count; (void)males; (void)females; }
void write_simulation_log(s_simulation_instance *sim, int sim_number, int initial_population) 
{ 
    (void)sim; (void)sim_number; (void)initial_population; 
}
void write_summary_log(int months, int initial_population, int nb_simulations,
                       s_simulation_results *all_results, uint64_t base_seed)
{
    (void)months; (void)initial_population; (void)nb_simulations; 
    (void)all_results; (void)base_seed;
}
#endif

// ===== END NEW LOGGING FUNCTIONS =====

/**
 * @brief Runs a full simulation of the rabbit population over a specified number of months.
 *        Initializes the population, then iteratively updates rabbit states each month.
 *        Tracks comprehensive statistics including population dynamics, sex distribution,
 *        and temporal patterns (peak/minimum populations).
 *        NEW: Now records monthly statistics if logging is enabled.
 * 
 * @param sim A pointer to the s_simulation_instance.
 * @param months The total number of months to simulate.
 * @param initial_population_nb The initial number of rabbits. If 2, "super" rabbits are used.
 * @param rng A pointer to the PCG random number generator state.
 * @return A s_simulation_results struct containing all collected statistics.
 */
s_simulation_results simulate(s_simulation_instance *sim, int months, int initial_population_nb, pcg32_random_t *rng)
{
    // Initialize results structure with default values
    s_simulation_results results = {0};
    
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
        init_2_super_rabbits(sim, rng);
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
        
        // Check for extinction (all rabbits dead)
        if (sim->free_count == sim->rabbit_count)
        {
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
        
        // NEW: Record monthly statistics before updating
        #if defined(ENABLE_DATA_LOGGING) && ENABLE_DATA_LOGGING != 0
        record_monthly_stats(sim, m, current_alive, sim->sex_distribution[1], sim->sex_distribution[0]);
        #endif
        
        // Update all rabbits for this month (births, deaths, aging, etc.)
        update_rabbits(sim, rng);
    }

    // Calculate final population counts
    int final_alive = sim->rabbit_count - sim->free_count;
    
    // Populate results structure with collected data
    results.total_dead = sim->dead_rabbit_count;
    results.final_alive = final_alive;
    results.final_males = sim->sex_distribution[1];
    results.final_females = sim->sex_distribution[0];
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
 * @brief Runs multiple simulations in parallel using OpenMP to calculate average population statistics.
 *        Each simulation runs independently with its own random number generator.
 *        Aggregates results across all simulations and prints comprehensive statistics.
 *        NEW: Now logs detailed monthly data for the first MAX_SIMULATIONS_TO_LOG simulations
 *        and creates a summary file with results from all simulations.
 * 
 * @param months The number of months for each simulation.
 * @param initial_population_nb The initial number of rabbits for each simulation.
 * @param nb_simulation The total number of simulations to run.
 * @param base_seed A base seed for the random number generators, combined with thread ID for uniqueness.
 * @return void
 */
void multi_simulate(int months, int initial_population_nb, int nb_simulation, uint64_t base_seed)
{
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
    
    #if defined(ENABLE_DATA_LOGGING) && ENABLE_DATA_LOGGING != 0
    // Allocate array to store results from all simulations for summary file
    s_simulation_results *all_results = malloc(sizeof(s_simulation_results) * nb_simulation);
    if (!all_results)
    {
        LOG_PRINT("Warning: Could not allocate memory for results logging\n");
    }
    #endif

    LOG_PRINT("\n\r    Completed Simulations: %3d / %3d (%3.0f%%)", 0, nb_simulation, 0.0f);
    
    // Parallel loop - each iteration runs one simulation on a separate thread
    #pragma omp parallel for reduction(+ : total_population, total_dead_rabbits, total_extinction_month, nb_extinctions, total_males, total_females, total_peak_population, total_peak_month, total_min_population, total_min_month, total_avg_population_sum)
    for (int i = 0; i < nb_simulation; i++)
    {
        // Create independent simulation instance and RNG for this thread
        s_simulation_instance sim_instance = {0};
        pcg32_random_t rng;
        
        // Seed RNG with base_seed combined with thread number for uniqueness
        pcg32_srandom_r(&rng, base_seed, (uint64_t)i);
        
        #if defined(ENABLE_DATA_LOGGING) && ENABLE_DATA_LOGGING != 0
        // Only log detailed monthly data for the first few simulations to avoid huge files
        if (i < MAX_SIMULATIONS_TO_LOG)
        {
            init_monthly_logging(&sim_instance, months);
        }
        #endif

        // Run single simulation and get results
        s_simulation_results results = simulate(&sim_instance, months, initial_population_nb, &rng);
        
        #if defined(ENABLE_DATA_LOGGING) && ENABLE_DATA_LOGGING != 0
        // Store results for summary file
        if (all_results)
        {
            all_results[i] = results;
        }
        
        // Write detailed log for this simulation if it was being tracked
        if (i < MAX_SIMULATIONS_TO_LOG && sim_instance.monthly_data)
        {
            write_simulation_log(&sim_instance, i + 1, initial_population_nb);
        }
        #endif

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

        // Clean up memory for this simulation
        reset_population(&sim_instance);

        // Progress tracking (thread-safe increment)
        #pragma omp atomic update
        sims_done++;

        // Only master thread prints progress to avoid garbled output
        int thread_id = omp_get_thread_num();
        if (PRINT_OUTPUT && thread_id == 0)
        {
            float progress = (float)sims_done * 100.0f / nb_simulation;
            LOG_PRINT("\r    Completed Simulations: %3d / %3d (%3.0f%%)", sims_done, nb_simulation, progress);
            fflush(stdout);
        }
    }

    // Final progress update showing 100% completion
    LOG_PRINT("\r    Completed Simulations: %3d / %3d (%3.0f%%)\n", nb_simulation, nb_simulation, 100.0f);
    
    #if defined(ENABLE_DATA_LOGGING) && ENABLE_DATA_LOGGING != 0
    // Write summary file with all simulation results
    if (all_results)
    {
        write_summary_log(months, initial_population_nb, nb_simulation, all_results, base_seed);
        free(all_results);
    }
    #endif

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

    // Print comprehensive results
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
}
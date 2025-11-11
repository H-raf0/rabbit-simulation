#include <math.h>
#include "rabbitsim.h"

// lance UNE simulation et renvoie un petit résumé
s_sim_result simulate_stats(s_simulation_instance *sim,
                            int months,
                            int initial_population_nb,
                            pcg32_random_t *rng)
{
    s_sim_result res = {0};

    if (initial_population_nb == 2)
        init_2_super_rabbits(sim, rng);
    else
        init_starting_population(sim, initial_population_nb, rng);

    int m;
    for (m = 0; m < months; ++m)
    {
        // tout le monde mort -> on arrête
        if (sim->free_count == sim->rabbit_count)
            break;
        update_rabbits(sim, rng);
    }

    int alive_rabbits_count = (int)(sim->rabbit_count - sim->free_count);

    res.alive_final = (double)alive_rabbits_count;
    res.dead_final  = (double)sim->dead_rabbit_count;
    res.extinct     = (alive_rabbits_count == 0);
    res.months_run  = m;

    return res;
}

// initialise l'accumulateur
void stats_acc_init(s_stats_acc *st)
{
    st->sum_alive = 0.0;
    st->sumsq_alive = 0.0;
    st->min_alive = 1e300;
    st->max_alive = -1e300;

    st->sum_dead = 0.0;
    st->sumsq_dead = 0.0;
    st->min_dead = 1e300;
    st->max_dead = -1e300;

    st->nb_extinctions = 0;
    st->n = 0;
}

// ajoute le résultat d'une simulation
void stats_acc_add(s_stats_acc *st, const s_sim_result *r)
{
    // alive
    st->sum_alive   += r->alive_final;
    st->sumsq_alive += r->alive_final * r->alive_final;
    if (r->alive_final < st->min_alive) st->min_alive = r->alive_final;
    if (r->alive_final > st->max_alive) st->max_alive = r->alive_final;

    // dead
    st->sum_dead   += r->dead_final;
    st->sumsq_dead += r->dead_final * r->dead_final;
    if (r->dead_final < st->min_dead) st->min_dead = r->dead_final;
    if (r->dead_final > st->max_dead) st->max_dead = r->dead_final;

    if (r->extinct) st->nb_extinctions++;

    st->n++;
}

// fusionne deux accumulateurs (utile avec OpenMP)
void stats_acc_merge(s_stats_acc *dst, const s_stats_acc *src)
{
    dst->sum_alive   += src->sum_alive;
    dst->sumsq_alive += src->sumsq_alive;
    if (src->min_alive < dst->min_alive) dst->min_alive = src->min_alive;
    if (src->max_alive > dst->max_alive) dst->max_alive = src->max_alive;

    dst->sum_dead   += src->sum_dead;
    dst->sumsq_dead += src->sumsq_dead;
    if (src->min_dead < dst->min_dead) dst->min_dead = src->min_dead;
    if (src->max_dead > dst->max_dead) dst->max_dead = src->max_dead;

    dst->nb_extinctions += src->nb_extinctions;
    dst->n              += src->n;
}

// affiche un petit rapport
void stats_print_report(const s_stats_acc *st,
                        int months,
                        int initial_population,
                        int nb_simulation)
{
    if (st->n == 0) {
        printf("No simulations.\n");
        return;
    }

    double n = (double)st->n;

    // alive
    double mean_alive = st->sum_alive / n;
    double var_alive  = (st->sumsq_alive / n) - (mean_alive * mean_alive);
    if (var_alive < 0) var_alive = 0;
    double sd_alive   = sqrt(var_alive);
    double ci_alive   = 1.96 * sd_alive / sqrt(n);  // 95%

    // dead
    double mean_dead = st->sum_dead / n;
    double var_dead  = (st->sumsq_dead / n) - (mean_dead * mean_dead);
    if (var_dead < 0) var_dead = 0;
    double sd_dead   = sqrt(var_dead);
    double ci_dead   = 1.96 * sd_dead / sqrt(n);

    double ext_percent = 100.0 * (double)st->nb_extinctions / n;

   
    printf("\n Input:\n");
    printf("  months              : %d\n", months);
    printf("  initial population  : %d\n", initial_population);
    printf("  simulations         : %d\n", nb_simulation);

    printf("\nFinal alive rabbits:\n");
    printf("  mean                : %.2f\n", mean_alive);
    printf("  ecart-type             : %.2f\n", sd_alive);
    printf("  min / max           : %.0f / %.0f\n", st->min_alive, st->max_alive);
    printf("  95%% CI (mean)       : [%.2f ; %.2f]\n",
           mean_alive - ci_alive, mean_alive + ci_alive);

    printf("\nFinal dead rabbits:\n");
    printf("  mean                : %.2f\n", mean_dead);
    printf("  ecart-type            : %.2f\n", sd_dead);
    printf("  min / max           : %.0f / %.0f\n", st->min_dead, st->max_dead);
    printf("  95%% CI (mean)       : [%.2f ; %.2f]\n",
           mean_dead - ci_dead, mean_dead + ci_dead);

    printf("\nExtinctions:\n");
    printf("  count               : %d\n", st->nb_extinctions);
    printf("  percent             : %.2f%%\n", ext_percent);

    printf("\n");
}

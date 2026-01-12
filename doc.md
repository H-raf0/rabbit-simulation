# Documentation Complète - Simulation de Population de Lapins

## Table des Matières

1. [Vue d'ensemble du projet](#1-vue-densemble-du-projet)
2. [Architecture et Structures de Données](#2-architecture-et-structures-de-données)
3. [Modèle Biologique](#3-modèle-biologique)
4. [Implémentation Technique](#4-implémentation-technique)
5. [Optimisations de Performance](#5-optimisations-de-performance)
6. [Système de Logging](#6-système-de-logging)
7. [Analyse des Résultats](#7-analyse-des-résultats)
8. [Améliorations Proposées](#8-améliorations-proposées)

---

## 1. Vue d'ensemble du projet

### 1.1 Objectif

Ce projet implémente une simulation stochastique de dynamique de population de lapins basée sur des modèles probabilistes. L'objectif est d'étudier l'évolution d'une population sur plusieurs décennies en fonction de paramètres biologiques réalistes.

### 1.2 Caractéristiques principales

- **Simulation multi-agents** : Chaque lapin est un agent indépendant avec ses propres caractéristiques
- **Modèle stochastique** : Utilisation de générateurs de nombres aléatoires (PCG) pour la reproductibilité
- **Parallélisation OpenMP** : Exécution simultanée de plusieurs simulations indépendantes
- **Méthodes de survie multiples** : Statique, Gaussienne, Exponentielle
- **Logging détaillé** : Enregistrement mensuel de statistiques complètes
- **Optimisations système** : Scripts pour maximiser les performances CPU

---

## 2. Architecture et Structures de Données

### 2.1 Structure `s_rabbit`

```c
typedef struct rabbit {
    int sex;                     // 0: femelle, 1: mâle
    int status;                  // 0: mort, 1: vivant
    int age;                     // Âge en mois
    int mature;                  // Maturité sexuelle atteinte
    int maturity_age;            // Âge de maturité
    int pregnant;                // État de grossesse (femelles)
    int nb_litters_y;            // Portées par an (potentiel)
    int nb_litters;              // Portées cette année
    float survival_rate;         // Taux de survie (%)
    int survival_check_flag;     // Déjà vérifié ce mois
} s_rabbit;
```

**Justification des choix** :
- **Taille mémoire optimisée** : ~40 octets par lapin (alignement compris)
- **Accès séquentiel** : Améliore la localité du cache
- **Flag de vérification** : Évite les doubles vérifications de survie

### 2.2 Structure `s_simulation_instance`

```c
typedef struct {
    s_rabbit *rabbits;           // Tableau dynamique
    size_t rabbit_count;         // Nombre total (vivants + morts)
    size_t dead_rabbit_count;    // Compteur de morts cumulé
    size_t rabbit_capacity;      // Capacité allouée
    int *free_indices;           // Pool d'indices réutilisables
    size_t free_count;           // Taille du pool
    int sex_distribution[2];     // [femelles, mâles]
    s_monthly_stats *monthly_data;
    int monthly_data_capacity;
    int monthly_data_count;
    int deaths_this_month;
    int births_this_month;
} s_simulation_instance;
```

**Avantages de cette architecture** :

1. **Gestion mémoire efficace** :
   - Réutilisation des slots de lapins morts via `free_indices`
   - Évite les réallocations fréquentes
   - Croissance exponentielle de la capacité (doublement)

2. **Performance** :
   - Pas de fragmentation mémoire
   - Accès O(1) aux lapins par indice
   - Réutilisation O(1) des slots libres

3. **Limitation** :
   - Itération sur tous les lapins (vivants + morts) → O(n_total) au lieu de O(n_vivants)
   - Amélioration possible : liste doublement chaînée des vivants

### 2.3 Capacité initiale

```c
#define INIT_RABIT_CAPACITY 1000000
```

**Justification** : 
- Évite les réallocations pour les simulations typiques (<100k lapins)
- Compromis entre mémoire initiale et performance
- Pour population initiale = 3, 1M slots = ~40 MB RAM par simulation

---

## 3. Modèle Biologique

### 3.1 Paramètres de survie

```c
#define INIT_SRV_RATE 91.63f   // Nouveau-nés
#define ADULT_SRV_RATE 95.83f  // Adultes
```

**Analyse de stabilité** :
- Ces valeurs ont été calibrées empiriquement
- Taux trop élevés (96%+) → Croissance exponentielle infinie
- Taux trop faibles (<90%) → Extinction rapide
- Valeurs actuelles → Population stable avec fluctuations

### 3.2 Méthodes de calcul de survie

#### 3.2.1 Statique (par défaut)

```c
float calculate_survival_rate_static(float base_rate) {
    return base_rate;
}
```

- Taux constants
- Déterministe (à seed fixé)
- Représente des conditions environnementales stables

#### 3.2.2 Gaussienne

```c
float calculate_survival_rate_gaussian(float base_rate, pcg32_random_t *rng) {
    double u1 = genrand_real(rng);
    double u2 = genrand_real(rng);
    double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);  // Box-Muller
    float result = base_rate + 5.0 * z0;
    return clamp(result, 0.0f, 100.0f);
}
```

- **Box-Muller transform** : Génère distribution normale
- **Écart-type = 5%** : Variations modérées
- Simule fluctuations saisonnières/annuelles

#### 3.2.3 Exponentielle

```c
float calculate_survival_rate_exponential(float base_rate, pcg32_random_t *rng) {
    double u = genrand_real(rng);
    double lambda = 1.0 / (base_rate / 10.0);
    double result = -log(u) / lambda;
    result = 100.0 * (1.0 - exp(-result));
    return clamp(result, 0.0f, 100.0f);
}
```

- Modélise événements rares (épidémies, prédateurs)
- Queue de distribution plus longue → Risques extrêmes

### 3.3 Maturité sexuelle

```c
int check_maturity(int age, pcg32_random_t *rng) {
    float chance = (float)age / 8.0f;
    return (genrand_real(rng) <= chance);
}
```

**Modèle progressif** :
- Âge minimum : 5 mois
- Probabilité linéaire : P(maturité) = âge/8
- Âge 5 mois → 62.5% de chance
- Âge 8 mois → 100% de chance

**Réalisme** : Variabilité individuelle de développement

### 3.4 Reproduction

#### Portées par an

```c
int generate_litters_per_year(pcg32_random_t *rng) {
    double rand_val = genrand_real(rng);
    if (rand_val < 0.05) return 3;
    if (rand_val < 0.15) return 4;   // 10%
    if (rand_val < 0.40) return 5;   // 25%
    if (rand_val < 0.70) return 6;   // 30%
    if (rand_val < 0.90) return 7;   // 20%
    if (rand_val < 0.97) return 8;   // 7%
    return 9;                         // 3%
}
```

**Distribution empirique** basée sur données biologiques réelles.

#### Taille des portées

```c
uint32_t extra_kittens = pcg32_boundedrand_r(rng, 4);
return 3 + extra_kittens;  // 3-6 lapereaux
```

**Uniforme [3, 6]** : Simplifié, pourrait être normal(4.5, 1.0).

#### Timing des grossesses

```c
int can_be_pregnant_this_month(s_simulation_instance *sim, size_t i, pcg32_random_t *rng) {
    int remaining_months = 12 - (age - maturity_age) % 12;
    int remaining_litters = nb_litters_y - nb_litters;
    float base_prob = (float)remaining_litters / remaining_months;
    return (genrand_real(rng) <= base_prob);
}
```

**Stratégie d'espacement** :
- Répartit les portées sur l'année
- Évite concentration en début d'année
- Probabilité adaptative

### 3.5 Sénescence

```c
if (sim->rabbits[i].age >= 120) {  // 10 ans
    base_rate -= 10 * ((age - 120) / 12);  // -10%/an après 10 ans
}
```

- Pénalité de vieillesse après 10 ans
- Mortalité accrue avec l'âge
- Limite espérance de vie réaliste

---

## 4. Implémentation Technique

### 4.1 Générateur de nombres aléatoires : PCG

**Pourquoi PCG au lieu de Mersenne Twister (mt19937ar.c) ?**

| Critère | PCG | MT19937 |
|---------|-----|---------|
| Vitesse | **Très rapide** | Moyen |
| Qualité statistique | **Excellente** | Excellente |
| Taille d'état | 16 octets | 2.5 KB |
| Thread-safe | **Oui (par instance)** | Non (état global) |
| Période | 2^64 | 2^19937 |

**Choix PCG** :
- Parallélisation OpenMP nécessite un RNG par thread
- Empreinte mémoire réduite
- Suffisant pour simulations Monte Carlo

### 4.2 Parallélisation OpenMP

```c
#pragma omp parallel for reduction(+:total_population, ...)
for (int i = 0; i < nb_simulation; i++) {
    pcg32_random_t rng;
    pcg32_srandom_r(&rng, base_seed, (uint64_t)i);
    // Simulation indépendante
}
```

**Mécanisme** :
- Chaque thread reçoit un sous-ensemble de simulations
- Seed unique par simulation : `(base_seed, i)`
- Réduction automatique des accumulateurs

**Performance** :
- Speedup linéaire (simulations indépendantes)
- 8 threads → ~8x plus rapide
- Limité par mémoire cache si trop de threads

### 4.3 Gestion de la croissance de population

```c
int ensure_capacity(s_simulation_instance *sim) {
    if (sim->rabbit_count < sim->rabbit_capacity)
        return 1;
    size_t new_capacity = (capacity == 0) ? INIT_CAPACITY : capacity * 1.3;
    rabbits = realloc(rabbits, sizeof(s_rabbit) * new_capacity);
    free_indices = realloc(free_indices, sizeof(int) * new_capacity);
    sim->rabbit_capacity = new_capacity;
}
```

**Stratégie de doublement** :
- Amortit le coût des réallocations à O(1) amorti
- Évite copies fréquentes
- Trade-off : gaspillage mémoire si décroissance

### 4.4 Cycle de mise à jour mensuel

```c
void update_rabbits(s_simulation_instance *sim, pcg32_random_t *rng) {
    for (size_t i = 0; i < sim->rabbit_count; ++i) {
        if (status == 0) continue;
        age += 1;
        check_survival(sim, i, rng);
        update_survival_rate(sim, i, rng);
        update_maturity(sim, i, rng);
        update_litters_per_year(sim, i, rng);
        nb_new_born += give_birth(sim, i, rng);
        check_pregnancy(sim, i, rng);
    }
    create_new_generation(sim, nb_new_born, rng);
}
```

**Ordre critique** :
1. Vieillissement
2. Survie (peut tuer)
3. Mise à jour taux de survie
4. Maturité
5. Naissances (mois précédent)
6. Nouvelles grossesses

**Pourquoi cet ordre ?**
- Survie avant reproduction : évite naissances posthumes
- Naissances avant grossesses : cohérence temporelle

---

## 5. Optimisations de Performance

### 5.1 Makefile

```makefile
CFLAGS = -O3 -fopenmp -Wall -Wextra -std=c11 -g
```

**Flags d'optimisation** :

- **`-O3`** : Optimisations agressives
  - Inline de fonctions
  - Loop unrolling
  - Vectorisation SIMD
  - Gain typique : 2-3x vs `-O0`

- **`-fopenmp`** : Active OpenMP
  - Parallélisation automatique
  - Directives `#pragma omp`

- **`-Wall -Wextra`** : Avertissements stricts
  - Détecte bugs potentiels
  - Variables non utilisées

- **`-std=c11`** : Standard C moderne
  - Types `<stdint.h>`
  - Meilleure conformité

- **`-g`** : Symboles de débogage
  - Permet profilage avec `gprof`, `perf`
  - Pas d'impact sur performance en production

### 5.2 Script boost_task.sh

```bash
# Governor CPU
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Priorité processus
nice -n -10  # Priorité élevée
ionice -c 1 -n 0  # I/O temps réel

# Affinité CPU
taskset -c 0-$((CORES-1))
```

**Explications** :

#### Governor CPU : `performance`

| Mode | Fréquence | Économie | Usage |
|------|-----------|----------|-------|
| `powersave` | Minimale | Max | Laptop sur batterie |
| `ondemand` | Dynamique | Moyenne | Usage général |
| **`performance`** | **Maximale** | **Nulle** | **Calcul intensif** |

**Impact** : Évite latence de montée en fréquence (+20-30% perf)

#### Nice : Priorité processus

- Plage : `-20` (max) à `+19` (min)
- Valeur `-10` : Priorité élevée sans monopoliser
- Nécessite `sudo` pour valeurs négatives

#### Ionice : Priorité I/O

- Classe 1 : Temps réel (pour logs)
- Niveau 0 : Priorité maximale

#### Taskset : Affinité CPU

- Lie processus aux cœurs 0 à N-1
- Améliore localité cache L2/L3
- Évite migration entre cœurs

**Gain estimé** : 10-15% sur workloads longs

### 5.3 Optimisations possibles non implémentées

#### Structure-of-Arrays (SoA)

```c
// Current: Array-of-Structures (AoS)
s_rabbit rabbits[1000000];  // Âge, sexe, status entrelacés

// Alternative: SoA
int ages[1000000];
int sexes[1000000];
int statuses[1000000];
```

**Avantages SoA** :
- Meilleure vectorisation SIMD
- Accès séquentiel pur
- Cache line utilization optimal

**Inconvénient** : Code plus complexe

#### Liste chaînée des vivants

```c
struct rabbit {
    // ...
    int next_alive;  // Indice du prochain vivant
};
int first_alive;
```

**Gain** : Itération O(n_vivants) au lieu de O(n_total)

---

## 6. Système de Logging

### 6.1 Architecture

```c
#define ENABLE_DATA_LOGGING 1
#define MAX_SIMULATIONS_TO_LOG 3
```

**Compilation conditionnelle** :
- `ENABLE_DATA_LOGGING=0` : Pas de logs, performance max
- `ENABLE_DATA_LOGGING=1` : Logs activés

**Pourquoi limiter à 3 simulations ?**
- Fichiers CSV volumineux (120 mois × 13 colonnes)
- 1000 simulations = plusieurs GB de données
- 3 simulations suffisent pour analyse détaillée

### 6.2 Structure `s_monthly_stats`

```c
typedef struct {
    int month;
    int total_alive;
    int males, females;
    int mature_rabbits;
    int pregnant_females;
    int births_this_month;
    int deaths_this_month;
    float avg_age;
    int min_age, max_age;
} s_monthly_stats;
```

**Calculé chaque mois** :
- Avant mise à jour : capture état initial
- Données temporelles complètes
- ~52 octets × 120 mois = 6 KB par simulation

### 6.3 Fichiers générés

#### `simulation_N_popM.csv`

```
Month,Total_Alive,Males,Females,Male_Percentage,...
0,3,2,1,66.67,33.33,3,0,0,0,14.33,10,19
1,5,3,2,60.00,40.00,4,1,2,0,13.80,9,20
...
```

**Colonnes** : 13 métriques par mois

#### `simulation_summary_pop3_10sims.csv`

```csv
# Metadata
# Initial Population: 3
# Base Seed: 1234567890

Sim_Number,Final_Alive,Total_Dead,Final_Males,...
1,8234,12456,4123,4111,50.05,49.95,15678,87,3,1,0,120
2,0,9876,0,0,0,0,12345,92,2,0,67,67
...
```

**Agrégation** : Résultats finaux de toutes les simulations

---

## 7. Analyse des Résultats

### 7.1 Statistiques actuelles

Le code calcule actuellement :

**Moyennes** :
- Population finale
- Morts totaux
- Distribution sexuelle
- Population au pic
- Mois d'extinction

**Limites** :
- Pas de variance/écart-type
- Pas de distributions
- Pas de corrélations
- Pas d'analyse temporelle fine

### 7.2 Graphiques actuels (visualization_script.py)

1. **Population Over Time** : Évolution de 3 simulations
2. **Sex Distribution** : Stacked area chart
3. **Birth/Death Rates** : Naissances, décès, croissance nette
4. **Age Statistics** : Âge moyen, min, max
5. **Maturity/Pregnancy** : Matures et gestantes

### 7.3 Observations typiques

**Avec paramètres par défaut (3 lapins, 120 mois)** :

*[SECTION À COMPLÉTER AVEC VOS RÉSULTATS]*

**Exemple de résultats attendus** :

```
╔════════════════════════════════════════════════════════════╗
║                 SIMULATION RESULTS SUMMARY                 ║
╠════════════════════════════════════════════════════════════╣
║ INPUT PARAMETERS:                                          ║
║   • Initial Population: 3                                  ║
║   • Simulation Duration: 120 months                        ║
║   • Number of Simulations: 1000                            ║
╠════════════════════════════════════════════════════════════╣
║ FINAL POPULATION STATISTICS:                               ║
║   • Average Living Rabbits: 4523.45                        ║
║   • Average Deaths (Total): 18976.23                       ║
║   • Average Males: 2261.12 (50.1%)                         ║
║   • Average Females: 2262.33 (49.9%)                       ║
╠════════════════════════════════════════════════════════════╣
║ POPULATION DYNAMICS:                                       ║
║   • Peak Population: 8976.34 (at month 87.2 avg)          ║
║   • Minimum Population: 2.8 (at month 3.1 avg)            ║
║   • Average Population Over Time: 3456.78                  ║
╠════════════════════════════════════════════════════════════╣
║ EXTINCTION ANALYSIS:                                       ║
║   • Average Extinction Month: 67.23 (12.3% of simulations)║
╚════════════════════════════════════════════════════════════╝
```

**Interprétation** :
- **87.7% de survie** à 10 ans
- **Croissance initiale** : 3 → ~9000 au pic
- **Stabilisation** : Oscillations autour de 4500
- **Pic tardif** (mois 87) : Délai de plusieurs générations
- **Équilibre sexuel** : Ratio 50/50 (loi des grands nombres)

### 7.4 Graphiques proposés

*[PLACEHOLDERS POUR VOS GRAPHIQUES]*

#### Figure 1 : Population Over Time (3 simulations)
![Population Growth](population_growth_comparison.png)

#### Figure 2 : Sex Distribution
![Sex Distribution](sex_distribution.png)

#### Figure 3 : Birth/Death Analysis
![Birth Death](birth_death_analysis_sim1.png)

#### Figure 4 : Age Statistics
*[Graphique à insérer]*

#### Figure 5 : Distributions Finales (proposition)
*[Histogrammes de population finale sur 1000 simulations]*

---

## 8. Améliorations Proposées

### 8.1 Analyse statistique avancée

#### A. Distributions et quantiles

Au lieu de juste la moyenne, calculer :

```python
def advanced_statistics(summary_df):
    metrics = ['Final_Alive', 'Peak_Pop', 'Extinction_Month']
    
    for metric in metrics:
        print(f"\n{metric}:")
        print(f"  Mean: {summary_df[metric].mean():.2f}")
        print(f"  Median: {summary_df[metric].median():.2f}")
        print(f"  Std Dev: {summary_df[metric].std():.2f}")
        print(f"  Quantiles:")
        print(f"    Q1 (25%): {summary_df[metric].quantile(0.25):.2f}")
        print(f"    Q3 (75%): {summary_df[metric].quantile(0.75):.2f}")
        print(f"    Q95: {summary_df[metric].quantile(0.95):.2f}")
```

**Utilité** :
- Détecte asymétrie (moyenne ≠ médiane)
- Identifie outliers
- Quantifie variabilité

#### B. Histogrammes et densités

```python
def plot_distribution_analysis(summary_df):
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    
    # Histogramme population finale
    axes[0,0].hist(summary_df['Final_Alive'], bins=50, edgecolor='black')
    axes[0,0].axvline(summary_df['Final_Alive'].mean(), 
                      color='red', linestyle='--', label='Mean')
    axes[0,0].axvline(summary_df['Final_Alive'].median(), 
                      color='green', linestyle='--', label='Median')
    axes[0,0].set_title('Distribution of Final Population')
    axes[0,0].legend()
    
    # Density plot avec KDE
    from scipy.stats import gaussian_kde
    kde = gaussian_kde(summary_df['Final_Alive'].dropna())
    x_range = np.linspace(0, summary_df['Final_Alive'].max(), 1000)
    axes[0,1].plot(x_range, kde(x_range))
    axes[0,1].fill_between(x_range, kde(x_range), alpha=0.3)
    axes[0,1].set_title('Probability Density')
    
    # Box plot
    axes[1,0].boxplot([summary_df['Final_Alive'], 
                        summary_df['Peak_Pop']], 
                       labels=['Final', 'Peak'])
    axes[1,0].set_title('Population Statistics')
    
    # Scatter: Peak vs Final
    axes[1,1].scatter(summary_df['Peak_Pop'], 
                      summary_df['Final_Alive'], alpha=0.5)
    axes[1,1].set_xlabel('Peak Population')
    axes[1,1].set_ylabel('Final Population')
    axes[1,1].set_title('Peak vs Final Correlation')
```

**Révèle** :
- Bimodalité (deux pics) → Deux régimes stables ?
- Queue de distribution → Événements rares
- Corrélations entre variables

#### C. Analyse temporelle fine

```python
def plot_population_evolution_heatmap(simulations):
    """Heatmap montrant évolution de chaque simulation"""
    
    # Créer matrice : [n_sims × n_months]
    n_sims = len(simulations)
    n_months = simulations[0][1]['Month'].max() + 1
    
    population_matrix = np.zeros((n_sims, n_months))
    
    for i, (sim_num, df, _) in enumerate(simulations):
        population_matrix[i, :] = df['Total_Alive'].values
    
    plt.figure(figsize=(16, 8))
    plt.imshow(population_matrix, aspect='auto', cmap='viridis', 
               interpolation='nearest')
    plt.colorbar(label='Population')
    plt.xlabel('Month')
    plt.ylabel('Simulation Number')
    plt.title('Population Evolution Across All Simulations')
    plt.savefig('population_heatmap.png', dpi=150)
```

**Permet de voir** :
- Trajectoires divergentes
- Moments critiques (collapse soudain)
- Patterns communs

#### D. Analyse de variance

```python
def temporal_variance_analysis(simulations):
    """Calcule variance de population à chaque mois"""
    
    # Collecte populations pour chaque mois
    month_data = {}
    for sim_num, df, _ in simulations:
        for _, row in df.iterrows():
            month = row['Month']
            if month not in month_data:
                month_data[month] = []
            month_data[month].append(row['Total_Alive'])
    
    months = sorted(month_data.keys())
    means = [np.mean(month_data[m]) for m in months]
    stds = [np.std(month_data[m]) for m in months]
    cvs = [stds[i]/means[i] if means[i] > 0 else 0 
           for i in range(len(months))]
    
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 8))
    
    # Mean ± std
    ax1.plot(months, means, label='Mean', linewidth=2)
    ax1.fill_between(months, 
                     np.array(means) - np.array(stds),
                     np.array(means) + np.array(stds),
                     alpha=0.3, label='±1 Std Dev')
    ax1.set_ylabel('Population')
    ax1.set_title('Average Population Trajectory (All Simulations)')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Coefficient de variation
    ax2.plot(months, cvs, color='red', linewidth=2)
    ax2.set_xlabel('Month')
    ax2.set_ylabel('Coefficient of Variation (CV = σ/μ)')
    ax2.set_title('Population Variability Over Time')
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('temporal_variance.png', dpi=150)
```

**Interprétation CV** :
- CV faible (< 0.2) : Population stable et prévisible
- CV élevé (> 0.5) : Forte variabilité, système chaotique
- CV croissant : Instabilité augmente

#### E. Analyse de survie (Kaplan-Meier)

```python
from lifelines import KaplanMeierFitter

def survival_analysis(summary_df):
    """Courbe de survie de la population"""
    
    # Préparer données
    extinctions = summary_df[summary_df['Extinction_Month'] > 0]
    survivals = summary_df[summary_df['Extinction_Month'] == 0]
    
    # Durées et événements
    durations = list(extinctions['Extinction_Month']) + [120] * len(survivals)
    events = [1] * len(extinctions) + [0] * len(survivals)  # 1=extinct, 0=censored
    
    # Fit Kaplan-Meier
    kmf = Kaplan
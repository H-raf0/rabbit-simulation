#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#include "rabbitsim.h"
#include "pcg_basic.h"


// Helper function to clear invalid input from stdin
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main()
{
    int exit_program = 0;
    int user_choice;

    int months = 240;
    int initial_population = 100000;
    int nb_simulations = 100;
    
    //uint64_t base_seed = (uint64_t)time(NULL) ^ (uintptr_t)&main;
    uint64_t base_seed = 1234997890123456700ULL;
    int seed_is_custom = 0;

    if(!PRINT_OUTPUT){
        multi_simulate(months, initial_population, nb_simulations, base_seed);
        return 0;
    }
    printf("Welcome to the Rabbit Simulation\n");

    while (!exit_program)
    {
        printf("\n----------------------------------------\n");
        printf("Main Menu\n");
        printf("----------------------------------------\n");
        printf("Current Settings:\n");
        printf("  - Months: %d\n", months);
        printf("  - Population: %d\n", initial_population);
        printf("  - Simulations: %d\n", nb_simulations);
        printf("  - Seed: %" PRIu64 " (%s)\n", base_seed, seed_is_custom ? "User-Defined" : "Random");
        
        printf("\nWhat do you want to do?\n");
        printf("    1. Change Simulation Parameters\n"
               "    2. Set a Custom Seed\n"
               "    3. Start Simulation\n"
               "    4. Exit\n"
               "Answer: ");
        
        if (scanf("%d", &user_choice) != 1) {
            clear_input_buffer();
            user_choice = -1; // Force default case
        } else {
            // It's good practice to clear the buffer even on success 
            // to handle inputs like "123xyz"
            clear_input_buffer(); 
        }

        printf("\n");

        switch (user_choice)
        {
        case 1:
            printf("Enter new number of months: ");
            if (scanf("%d", &months) != 1) {
                printf("Invalid input. Parameter not changed.\n");
                clear_input_buffer();
            } else { clear_input_buffer(); }

            printf("Enter new initial population: ");
            if (scanf("%d", &initial_population) != 1) {
                printf("Invalid input. Parameter not changed.\n");
                clear_input_buffer();
            } else { clear_input_buffer(); }

            printf("Enter new number of simulations: ");
            if (scanf("%d", &nb_simulations) != 1) {
                printf("Invalid input. Parameter not changed.\n");
                clear_input_buffer();
            } else { clear_input_buffer(); }

            printf("Parameters updated.\n");
            break;

        case 2:
            printf("Please enter a 64-bit unsigned integer for the seed: ");
            if (scanf("%" SCNu64, &base_seed) != 1) {
                printf("Invalid input. Seed not changed.\n");
                clear_input_buffer();
            } else {
                seed_is_custom = 1;
                printf("Seed has been set.\n");
                clear_input_buffer();
            }
            break;

        case 3:
            printf("--> Starting simulation with the current settings...\n");
            multi_simulate(months, initial_population, nb_simulations, base_seed);
            printf("\n\n--> Simulation finished.\n");
            break;

        case 4:
            exit_program = 1;
            printf("Exiting simulation. Goodbye!\n");
            break;

        default:
            printf("Invalid answer! Please choose an option from 1 to 4.\n");
            break;
        }
    }

    return 0;
}
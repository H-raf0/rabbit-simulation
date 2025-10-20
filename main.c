#include "rabbitsim.h"
#include "mt19937ar.h"



// TO DO: size_t is not good probably

int main()
{
    /*
    unsigned long init[4]={0x123, 0x234, 0x345, 0x456}, length=4;
    init_by_array(init, length);
    */
    init_genrand(12345UL);
    // int generations;
    // printf("Number of generations :");scanf("%d", &generations);
    // printf(fibonacci(generations));

    simulate(120, 10000);
    return 0;
}
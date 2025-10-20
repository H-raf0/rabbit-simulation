#include "rabbitsim.h"
#include "mt19937ar.h"


int main()
{
    /*
    unsigned long init[4]={0x123, 0x234, 0x345, 0x456}, length=4;
    init_by_array(init, length);
    */
    init_genrand(123912UL);
    // int generations;
    // printf("Number of generations :");scanf("%d", &generations);
    // printf(fibonacci(generations));

    //simulate(240, 100000);
    multi_simulate(240, 10000, 100);
    return 0;
}

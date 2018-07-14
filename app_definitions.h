#ifndef __APP_DEFINITIONS_H__
    #define __APP_DEFINITIONS_H__

    //Local definitions


    //Define only one - Opt for real-time (1x or 10x time-lapse) or processor-limited simulation
    //#define REAL_TIME_1X              //Real time, 1 elapsed second means 1 second of traffic simulation
    //#define REAL_TIME_10X             //Real time, 1 elapsed second means 10 second of traffic simulation
    #define PROCESSOR_LIMITED         //Run the simulation as fast as possible, limited to the computer capacity, single core usage

    //Define only one - Two test scenarios
    //#define SCENARIO_1                  //Scenario 1, constant turn rate, 1 lane per street
    #define SCENARIO_2                //Scenario 2, turn rate of 10-35%, 1-4 lanes per street

    //Define only one - Two control strategies
    //#define IC_CONTROL                  //Traffic lights are independent with their own timers
    #define CBCL_CONTROL                //Traffic lights change operation based on block occupation

    //Define only one - Multi-threaded control
    //#define SINGLE_THREAD //2.3s
    //#define MULTI_THREAD //4.9s
    #define MULTI_THREAD_POOL //Ver sync


    //Define only one - types of syncing between threads
    #ifdef MULTI_THREAD_POOL
        //#define HARD_SYNC //1.9s
        //#define SOFT_SYNC //1.65s
        #define NO_SYNC //1.3s
    #endif

    //Define only one - Traffic intensity
    //#define CAR_RATE_LINEAR   1       //The number means 0.1cars/lane/entrypoint/second
    //#define CAR_RATE_LINEAR   2       //The number means 0.2cars/lane/entrypoint/second
    //#define CAR_RATE_LINEAR   3       //The number means 0.3cars/lane/entrypoint/second
    //#define CAR_RATE_LINEAR   4       //The number means 0.4cars/lane/entrypoint/second
    //#define CAR_RATE_LINEAR   5       //The number means 0.5cars/lane/entrypoint/second
    //#define CAR_RATE_LINEAR_GROWTH    //Starts with 0.1cars/lane/entrypoint/second and finishes at 0.5cars/lane/entrypoint/second
    #define CAR_RATE_POISSON            //Does a simplified poisson distribution of car rate input. From 0.1 to a peak of 0.5cars/lane/entrypoint/second

    //Simulation length
    #define SIM_TIME_MS (2000*1000)     //Time in ms for the simulation - Not to exceed 2000s without increasing car rates on app_definitions.c

    //car.h dependencies
    #define CAR_SIZE          4         //Car size in meters

    //street.h dependencies
    #define N_STREETS_W_E    10         //Amount of streets from West to East - Not to exceed 10 without increasing lane counts on app_definitions.c
    #define N_STREETS_N_S    10         //Amount of streets from North to South - Not to exceed 10 without increasing lane counts on app_definitions.c
    #define BLOCK_SIZE      100         //Length of block in meters REQ: E
    #define MAX_LANES         4         //Max number of lanes per street

    //light.h dependencies              //REQ: O
    #define LIGHTS_RED_TIME        47   //Time in seconds in red
    #define LIGHTS_GREEN_TIME      38   //Time in seconds in green
    #define LIGHTS_YELLOW_TIME     5    //Time in seconds in yellow
    #define LIGHTS_DEAD_TIME       1    //Time in seconds where both lights are red

    //CBCL control definitions
    #define LIGHTS_RED_TIME_REDUCE_RED   24 //Time to make traffic light cycle shorter
    #define LIGHTS_RED_TIME_NO_CHANGE    39 //Time to avoid changing since it is close from being green
    #define LIGHTS_RED_TIME_REDUCED      30 //Time in seconds for red lights when block has 60% or more occupation

#endif                                  // __APP_DEFINITIONS_H__

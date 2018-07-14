#ifndef __LIGHT_H__
    #define __LIGHT_H__

    //External libs
    #include <stdlib.h>

    //Locals
    #include "app_definitions.h"

    //Definitions
    #ifndef LIGHTS_RED_TIME
        #error no LIGHTS_RED_TIME definition
    #endif

    #ifndef LIGHTS_GREEN_TIME
        #error no LIGHTS_GREEN_TIME definition
    #endif

    #ifndef LIGHTS_YELLOW_TIME
        #error no LIGHTS_YELLOW_TIME definition
    #endif

    #ifndef LIGHTS_DEAD_TIME
        #error no LIGHTS_DEAD_TIME definition
    #endif

    #define N_LIGHTS (N_STREETS_W_E*N_STREETS_N_S) //Amount of traffic lights based on the amount of street crossings

    //Data sets
    enum light_state
    {
        GREEN = 0,
        RED,
        YELLOW,
        INVALID
    };

    enum light_pace
    {
        NORMAL  = 0,
        FASTER,
        CHANGED
    };

    enum light_direction
    {
        NORTH_SOUTH = 0,
        WEST_EAST
    };

    struct light
    {
        int timer;
        enum light_pace pace;
        enum light_state light_n_s;
        enum light_state light_w_e;
    };

    //Functions
    void init_light(struct light *l_in);
    void update_light_1s(struct light *l_in);
    void reduce_red_time(struct light *l_in, enum light_direction dir);

#endif

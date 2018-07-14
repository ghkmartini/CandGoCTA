#include "light.h"

#define LIGHT_TIMESPAN              (LIGHTS_RED_TIME            + LIGHTS_GREEN_TIME + LIGHTS_YELLOW_TIME)
//#define LIGHTS_RED_TIME_REDUCED     (LIGHTS_RED_TIME_REDUCED    + LIGHTS_GREEN_TIME + LIGHTS_YELLOW_TIME)
#define LIGHT_TIMESPAN_REDUCE_RED   (LIGHTS_RED_TIME_REDUCE_RED + LIGHTS_GREEN_TIME + LIGHTS_YELLOW_TIME)
#define LIGHT_TIMESPAN_NO_CHANGE    (LIGHTS_RED_TIME_NO_CHANGE  + LIGHTS_GREEN_TIME + LIGHTS_YELLOW_TIME)

void init_light(struct light *l_in)                                     //Initializes a traffic light in the map
{
    l_in->pace = NORMAL;                                                //All lights operate in normal pace
    l_in->timer = rand()%LIGHT_TIMESPAN;                                //Start the light timer with a random number in between the timespan
    update_light_1s(l_in);                                              //Update the dataset
}

void update_light_1s(struct light *l_in)                                //Function to be called every 1s, update all light states REQ H
{
    enum light_state* temp_light_1;
    enum light_state* temp_light_2;


    if(l_in->timer < (LIGHT_TIMESPAN/2))                                //Swaps the lights of the same crossing to avoid code repetition
    {
        temp_light_1 = &l_in->light_n_s;                                //REQ: H
        temp_light_2 = &l_in->light_w_e;
    }
    else
    {
        temp_light_1 = &l_in->light_w_e;
        temp_light_2 = &l_in->light_n_s;
    }

    //Do a Green-Yellow-Red logic with dead time for a pair of crossing lights
         if((l_in->timer%(LIGHT_TIMESPAN/2)) < LIGHTS_DEAD_TIME)  *temp_light_1 = RED;  //REQ: I
    else if((l_in->timer%(LIGHT_TIMESPAN/2)) < LIGHTS_GREEN_TIME) *temp_light_1 = GREEN;
    else                                                          *temp_light_1 = YELLOW;

    *temp_light_2 = RED;

    l_in->timer = (l_in->timer+1)%LIGHT_TIMESPAN;                                                            //Increase the light timer as a periodic function does

    //REQ: #3 & #4 CBCL
    if(l_in->pace == FASTER &&                                                                               //If there is a requirement to speed up this change of light
       l_in->timer > LIGHTS_DEAD_TIME &&                                                                     //If it is not on the dead time
       l_in->timer < (LIGHT_TIMESPAN/2))                                                                     //If it is on the first half of the counting, i.e. speed up red for WEST and EAST crossings
    {
        l_in->timer += (LIGHTS_RED_TIME - LIGHTS_RED_TIME_REDUCED);                                          //Speed up this cycle
        if(l_in->timer > (LIGHT_TIMESPAN/2)) l_in->timer = (LIGHT_TIMESPAN/2);                               //Saturate in the semi-cycle
        l_in->pace = CHANGED;                                                                                 //Clear the speed up flag
    }

    if(l_in->pace == FASTER &&                                                                               //If there is a requirement to speed up this change of light
       l_in->timer > LIGHTS_DEAD_TIME &&                                                                     //If it is not on the dead time
       l_in->timer >= (LIGHT_TIMESPAN/2))                                                                    //If it is on the second half of the counting, i.e. speed up red for NORTH and SOUTH crossings
    {
        l_in->timer += (LIGHTS_RED_TIME - LIGHTS_RED_TIME_REDUCED);                                          //Speed up this cycle
        if(l_in->timer > LIGHT_TIMESPAN) l_in->timer = LIGHT_TIMESPAN;                                       //Saturate in the semi-cycle
        l_in->pace = CHANGED;                                                                                 //Clear the speed up flag
    }

    if( l_in->pace == CHANGED &&
       (l_in->timer == 0 || l_in->timer == (LIGHT_TIMESPAN/2)) )
    {
        l_in->pace = NORMAL;
    }
}

void reduce_red_time(struct light *l_in, enum light_direction dir)
{
    if(dir == NORTH_SOUTH && l_in->light_n_s == RED && l_in->pace  == NORMAL &&                              //Light has to be red to take action -> REQ: #6 CBCL
       l_in->timer  > LIGHTS_DEAD_TIME  &&                                                                   //Take out the red light for the dead time to avoid buggy behavior
       l_in->timer <= LIGHT_TIMESPAN_NO_CHANGE )                                                             //Light can't be close from being green -> REQ: #5 CBCL
    {
        if(l_in->timer <= LIGHT_TIMESPAN_REDUCE_RED) l_in->pace = FASTER;                                    //REQ: #3 CBCL
        else                                         l_in->timer = LIGHT_TIMESPAN - LIGHTS_YELLOW_TIME;      //REQ: #4 CBCL
    }

    if(dir == WEST_EAST && l_in->light_w_e == RED && l_in->pace  == NORMAL &&                                //Light has to be red to take action -> REQ: #6 CBCL
       l_in->timer  > LIGHTS_DEAD_TIME  &&                                                                   //Take out the red light for the dead time to avoid buggy behavior
       l_in->timer <= LIGHTS_RED_TIME_NO_CHANGE )                                                            //Light can't be close from being green -> REQ: #5 CBCL
    {
        if(l_in->timer <= LIGHTS_RED_TIME_REDUCE_RED) l_in->pace = FASTER;                                   //REQ: #3 CBCL
        else                                          l_in->timer = LIGHTS_GREEN_TIME + LIGHTS_YELLOW_TIME;  //REQ: #4 CBCL
    }
}

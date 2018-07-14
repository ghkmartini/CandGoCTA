#include "traffic_manager.h"

#include "app_data_definitions.h"

#define CAR_BUFFER_SIZE 500000

struct car car_buffer[CAR_BUFFER_SIZE]; //Big buffer to avoid having to search for unused cars before re-inserting, done this way for simplicity's sake
int last_used_car_id;
int traffic_tick_1s;
int traffic_flow_tick;
int traffic_flow_counter;

void init_traffic_manager(void)
{
    int i = 0;
    traffic_tick_1s = 0;                //Starts the loops divider with a zero valued number
    traffic_flow_tick = 0;              //Starts the flow control tick counter with zero
    traffic_flow_counter = 0;           //Starts the 200ms counter with zero
    srand(time(NULL));                  //Give a seed number for the random number generator

    init_streets(2, (N_STREETS_W_E > N_STREETS_N_S ? N_STREETS_W_E : N_STREETS_N_S),
                 number_of_lanes_input, turn_rate_input);

    for(i=0; i<CAR_BUFFER_SIZE; i++)    //Starts all car with a unique ID and outside all streets.
    {
        car_buffer[i].id = i;
        car_buffer[i].on_map = 0;
    }
    last_used_car_id = 0;               //None of the cars are in use at the start of the program
}

void insert_cars_200ms()
{
    int i;


    #ifdef CAR_RATE_LINEAR               //Linear car rate input - constant flow
     traffic_flow_tick++;
     if( (traffic_flow_tick%(50/CAR_RATE_LINEAR)) == 0)
    #elif defined CAR_RATE_LINEAR_GROWTH //Linear increase of car inputs along the time
     int temp_calc;

     traffic_flow_counter++;
     traffic_flow_tick++;

     temp_calc = 1 + (traffic_flow_counter/2000);

     if( (traffic_flow_tick%(50/temp_calc) ) == 0)
    #elif defined CAR_RATE_POISSON       //Simplified poisson distribution curve along the time
     int temp_calc;

     traffic_flow_counter++;
     traffic_flow_tick++;

          if(traffic_flow_counter <= 2000) temp_calc = 1;
     else if(traffic_flow_counter <= 4000) temp_calc = 5;
     else if(traffic_flow_counter <= 6000) temp_calc = 4;
     else if(traffic_flow_counter <= 8000) temp_calc = 2;
     else                                  temp_calc = 1;

     if( (traffic_flow_tick%(50/temp_calc) ) == 0)
    #else                               //If there is no definition, make it equivalent to linear 5
     traffic_flow_tick++;
     if((traffic_flow_tick%10) == 0)
    #endif
    {
        traffic_flow_tick = 0;
        for(i=0; i<N_STREETS_W_E;i++) last_used_car_id += insert_car_all_lanes_by_coordenate(i, WEST , &car_buffer[last_used_car_id]);
        for(i=0; i<N_STREETS_N_S;i++) last_used_car_id += insert_car_all_lanes_by_coordenate(i, NORTH, &car_buffer[last_used_car_id]);
    }
}

void update_traffic_200ms(void)
{
    move_cars_200ms();      //REQ: G 4m every 200ms-> 20m/s
    insert_cars_200ms();

    traffic_tick_1s = (traffic_tick_1s+1)%5;
    if(traffic_tick_1s == 0)
    {
        #if defined(PROCESSOR_LIMITED) && defined(CBCL_CONTROL)
            calculate_car_occupation();
        #endif
        update_all_lights_1s();
    }

    #ifndef PROCESSOR_LIMITED
        calculate_car_occupation();
    #endif
}

int get_cars_passed_on_map()
{
    return last_used_car_id;
}

void delete_din_data(void)
{
    clear_data_pool();
}

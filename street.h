#ifndef __STREET_H__
    #define __STREET_H__

    //External libs
    #include <stdlib.h>
    #include <pthread.h>

    //Locals
    #include "app_definitions.h"
    #include "car.h"
    #include "light.h"
    #include "thpool.h"

    #ifndef N_STREETS_W_E               //Amount of streets from West to East
        #error no N_STREET_W_E definition
    #endif

    #ifndef N_STREETS_N_S               //Amount of streets from North to South
        #error no N_STREET_N_S definition
    #endif

    #ifndef BLOCK_SIZE                  //Length of block in meters
        #error no BLOCK_SIZE definition
    #endif

    #ifndef MAX_LANES                   //Max number of lanes per street
        #error no MAX_LANES definition
    #endif

    #if ((BLOCK_SIZE%CAR_SIZE)!= 0)     //Discretization of the car slots in the lanes REQ: F
        #error CAR_SIZE and BLOCK_SIZE must be multiples!
    #endif

    //Amount of cars that fit in a lane in a block
    #define SLOTS_PER_BLOCK (BLOCK_SIZE/CAR_SIZE)

    //Max number of blocks that a street can have based on the number of crossings
    #define MAX_BLOCKS      (N_STREETS_W_E > N_STREETS_N_S ? (N_STREETS_W_E+1) : (N_STREETS_N_S+1))

    //Max number of car slots per street lane
    #define MAX_SLOTS       (SLOTS_PER_BLOCK*MAX_BLOCKS)

    //Data sets
    enum street_direction
    {
        NORTH = 0,
        SOUTH,
        WEST,
        EAST
    };

    struct street
    {
        int  street_number;
        int  block_count;
        int  lane_count;                                    //REQ: D
        int  car_count;
        int  occupation[MAX_BLOCKS];
        enum street_direction direction;                    //REQ: C
        struct car *street_slots[MAX_LANES][MAX_SLOTS];
    };

    struct crossing
    {
        unsigned char turn_rate[2];
        struct street *street_w_e;
        struct street *street_n_s;
        struct light  *cross_light;
    };

    //Functions
    void init_streets(int directions, int n_streets,
                      const int lanes_in[directions][n_streets],
                      const int turn_rate_in[directions][n_streets][n_streets]);

    void clear_data_pool(void);

    int insert_car_all_lanes_by_coordenate(int id, enum street_direction dir, struct car *c_in);
    void move_cars_200ms(void);
    void calculate_car_occupation(void);
    void update_all_lights_1s(void);

    void get_street(struct street *s_in, enum street_direction dir, int street_id);
    void get_crossing(struct crossing *c_in, int st_w_e, int st_n_s);
    int  get_total_car_count_on_map(void);

#endif // __STREET_H__

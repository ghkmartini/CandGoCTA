#include "street.h"

struct street   streets_w_e[N_STREETS_W_E];                         //REQ: A
struct street   streets_n_s[N_STREETS_N_S];                         //REQ: B
struct crossing crossings[N_STREETS_W_E][N_STREETS_N_S];
struct light    lights[N_LIGHTS];
int    total_cars_on_map;

threadpool thpool;

void init_streets(int directions, int n_streets,
                  const int lanes_in[directions][n_streets],
                  const int turn_rate_in[directions][n_streets][n_streets])
{
    int h, i, j, k;                                                 //Iterators
    struct street *temp_street;                                     //Street pointer to handle north/south and west/east roads

    total_cars_on_map = 0;                                          //Set zero to the car counter to be calculated in the future
    for(i=0; i<N_LIGHTS; i++) init_light(&lights[i]);               //Starts traffic lights with different time counters


    for(h=0; h<2; h++)                                              //Loop through north/south and west/east roads
    {
        if(h==0) temp_street = streets_w_e;                         //One direction at a time
        else     temp_street = streets_n_s;

        for(i=0; i<((h==0)?(N_STREETS_W_E):(N_STREETS_N_S));i++)    //For every street in each direction
        {
            temp_street[i].street_number = i;                                           //Set a street ID
            temp_street[i].block_count = ((h==0)?(N_STREETS_N_S):(N_STREETS_W_E)) + 1;  //Set the amount of blocks - depends on the number of crossing streets
            temp_street[i].lane_count  = lanes_in[h][i];                                //Set the number of lanes for this street

            if(h == 0)
            {
                if((i%2) == 0) temp_street[i].direction = WEST;                         //Set the direction of the street, one to each direction, each time
                else           temp_street[i].direction = EAST;                         // REQ: L
            }

            if(h == 1)
            {
                if((i%2) == 0) temp_street[i].direction = NORTH;                        //Set the direction of the street, one to each direction, each time
                else           temp_street[i].direction = SOUTH;
            }

            for(j=0; j<MAX_BLOCKS; j++) temp_street[i].occupation[j] = 0;               //Starts with zero occupation of cars on all blocks for every street

            for(j=0; j<MAX_LANES; j++)
            {
                for(k=0; k<MAX_SLOTS; k++) temp_street[i].street_slots[j][k] = 0;       //All car slots are empty at the beginning of the simulation
            }
        }
    }

    for(i=0; i<N_STREETS_W_E; i++)                                  //Setup all crossing pointers, crossing lights and turn rates
    {
        for(j=0; j<N_STREETS_N_S; j++)
        {
            crossings[i][j].turn_rate[0] = turn_rate_in[0][i][j];
            crossings[i][j].turn_rate[1] = turn_rate_in[1][i][j];
            crossings[i][j].street_w_e  = &streets_w_e[i];
            crossings[i][j].street_n_s  = &streets_n_s[j];
            crossings[i][j].cross_light = &lights[((i+1)*(j+1)-1)];
        }
    }

    #ifdef MULTI_THREAD_POOL
        thpool = thpool_init(2);
    #endif
}

void clear_data_pool(void)
{
    #ifdef MULTI_THREAD_POOL
        thpool_destroy(thpool);
    #endif
}

void update_cars(struct street *s_in)
{
    int i,j,k;                                                                                                                    //Create loop iterators

    int dir, dir2;                                                                                                                //Saves the direction of the flow in the current and crossing
    int last_slot;                                                                                                                //Stores where the street begins/end depending on direction
    int x_slot;                                                                                                                   //Get the corresponding slot of the crossing street
    int turn_rate;                                                                                                                //Gets the turn rate from the current street to the crossing street

    struct street       *temp_street;                                                                                             //Creates temp structures for data handling
    struct crossing     temp_crossing;
    enum   light_state  temp_light_state = INVALID;

    if(s_in->direction == NORTH || s_in->direction == WEST) dir = +1;                                                             //Depending on traffic direction, changes the way car slots are handled
    else                                                    dir = -1;

    last_slot = (s_in->block_count*SLOTS_PER_BLOCK)-1;                                                                            //Calculates the number of the last slot of the street (southmost/eastmost point)

    for(i=0; i<MAX_LANES; i++)
    {
        if(i >= s_in->lane_count) break;                                                                                          //If the road doesn't have as many lanes, stop calculation

        if(dir > 0)                                                                                                               //If there is a car leaving the map, clears the slot and say the car is not on the map
        {
            if(s_in->street_slots[i][0] != 0) s_in->street_slots[i][0]->on_map = 0;
            s_in->street_slots[i][0] = s_in->street_slots[i][1];                                                                  //Pulls the next car to the last slot, don't check crossing since there isn't any
            s_in->street_slots[i][1] = 0;                                                                                         //Clear the slot that moved
        }
        else
        {
            if(s_in->street_slots[i][last_slot] != 0) s_in->street_slots[i][last_slot]->on_map = 0;
            s_in->street_slots[i][last_slot]   = s_in->street_slots[i][last_slot-1];                                              //Pulls the next car to the last slot, don't check crossing since there isn't any
            s_in->street_slots[i][last_slot-1] = 0;                                                                               //Clear the slot that moved
        }

        for( (dir>0)? (j = 1)        : (j = (last_slot-1));
             (dir>0)? (j < last_slot): (j > 0)            ;
             (dir>0)? (j++)          : (j--)              )
        {
            if(s_in->street_slots[i][j] != 0) continue;                                                                           //If there is a car on the actual car slot, proceed checking the other empty ones

            if(j!= 0 && (j%SLOTS_PER_BLOCK) == 0)                                                                                 //Check if it is on a crossing
            {
                if(s_in->direction == NORTH ||                                                                                    //Check the crossing lights before moving traffic
                   s_in->direction == SOUTH) get_crossing(&temp_crossing, (j/SLOTS_PER_BLOCK) - 1, s_in->street_number);          //Get the corresponding crossing which will bring the...
                else                         get_crossing(&temp_crossing,  s_in->street_number   ,(j/SLOTS_PER_BLOCK) - 1);       //...crossing street and both traffic lights

                if(s_in->direction == NORTH || s_in->direction == SOUTH) temp_light_state = temp_crossing.cross_light->light_n_s; //Every crossing has two traffic lights,
                else                                                     temp_light_state = temp_crossing.cross_light->light_w_e; //So this "if" catches the correspondent light

                if(temp_light_state == RED) continue;                                                                              //If the lights are red, don't move the car, proceed to the next car slot REQ: N

                                                                                                                                   //REQ: K
                if(s_in->street_slots[i][j] != 0 || s_in->street_slots[i][j-dir] != 0) continue;                                   //If the car slot after the crossing isn't empty, don't move the car, proceed to the next car slot
                                                                                                                                   //Note that the crossing street isn't checked since its lights are red and the "crossing box" is always empty due to this condition
                if(s_in->direction == NORTH || s_in->direction == SOUTH) turn_rate = temp_crossing.turn_rate[0];                   //Captures the turn rate base on street direction
                else                                                     turn_rate = temp_crossing.turn_rate[1];

                if( (rand()%100) < turn_rate)                                                                                      //Check if the car will turn to the other street REQ: J
                {
                    if(s_in->direction == NORTH || s_in->direction == SOUTH) temp_street = temp_crossing.street_w_e;               //Gets the crossing street
                    else                                                     temp_street = temp_crossing.street_n_s;

                    if(temp_street->direction == NORTH || temp_street->direction == WEST) dir2 = -1;                               //Gets the crossing street direction
                    else                                                                  dir2 = +1;                               //Note that the signal is inverted in relation to "dir"

                    x_slot = ((s_in->street_number+1)*SLOTS_PER_BLOCK);                                                            //Calculates the slot of the crossing street to make the turn

                    for(k=0; k<temp_street->lane_count; k++)                                                                       //Allows the turn to any lane, the first that is empty
                    {
                        if(temp_street->street_slots[k][x_slot] == 0 && temp_street->street_slots[k][x_slot+dir2] == 0)            //If the crossing is free of cars to make the turn
                        {
                            temp_street->street_slots[k][x_slot+dir2] = s_in->street_slots[i][j+dir];                              //Change the car to the other street
                            s_in->street_slots[i][j+dir] = 0;                                                                      //Clear the car from the coming street
                        }
                    }
                }
            }
            s_in->street_slots[i][j]   = s_in->street_slots[i][j+dir];                                                             //If it is not in a crossing, didn't turn and is not in a red light, move the car
            s_in->street_slots[i][j+dir] = 0;                                                                                      //Clears the previous slot where the car was
        }
    }
}

int insert_car(struct street *s_in, int lane_in, struct car *c_in)
{
    int input_slot;

    if(lane_in > s_in->lane_count) return 0;                                                                                       //If the call is trying to place a car on a lane that isn't there, return

    if(s_in->direction == NORTH || s_in->direction == WEST) input_slot = (s_in->block_count*SLOTS_PER_BLOCK)-1;                    //According to the mapping, places the car at the end of the road if it is headed north/west
    else                                                    input_slot = 0;                                                        //If not, put it at the beginning

    if(s_in->street_slots[lane_in][input_slot] != 0) return 0;                                                                     //If the targeted place isn't empty, return false
    s_in->street_slots[lane_in][input_slot] = c_in;                                                                                //Place the car at the start of the street
    return 1;                                                                                                                      //If the car was placed successfully, return true
}

int insert_car_all_lanes_by_coordenate(int id, enum street_direction dir, struct car *c_in)
{
    int i;
    int inserts = 0;
    struct street *temp_street;                                     //Street pointer to handle north/south and west/east roads

    if(dir == NORTH || dir == SOUTH)                                //Checks the street direction to get the right data set
    {
        if(id > (N_STREETS_N_S-1)) id = (N_STREETS_N_S-1);          //Saturates the Id in between the actual data set
        temp_street = streets_n_s;                                  //Get the pointer right
    }
    else
    {
        if(id > (N_STREETS_W_E-1)) id = (N_STREETS_W_E-1);          //Saturates the Id in between the actual data set
        temp_street = streets_w_e;                                  //Get the pointer right
    }

    for(i=0; i<temp_street[id].lane_count; i++)                     //Take a car from the input array and puts it in a street
    {
        inserts += insert_car(&temp_street[id], i, &c_in[i]);       //Place the car in its lane
    }

    return inserts;                                                 //Returns the amount of cars that were inserted
}

#ifdef MULTI_THREAD
    void *move_cars_w_e(void* var)
#else
    void move_cars_w_e(void)
#endif
{
    int i;
    for(i=0; i<N_STREETS_W_E; i++) update_cars(&streets_w_e[i]);
}

#ifdef MULTI_THREAD
    void *move_cars_n_s(void* var)
#else
    void move_cars_n_s(void)
#endif
{
    int i;
    for(i=0; i<N_STREETS_N_S; i++) update_cars(&streets_n_s[i]);
}

void move_cars_200ms(void)
{
    #ifdef SINGLE_THREAD
        int i;
        for(i=0; i<N_STREETS_N_S; i++) update_cars(&streets_n_s[i]);
        for(i=0; i<N_STREETS_W_E; i++) update_cars(&streets_w_e[i]);
    #endif

    #ifdef MULTI_THREAD
        pthread_t id1, id2;
        pthread_create(&id1, NULL, move_cars_w_e, NULL);
        pthread_create(&id2, NULL, move_cars_n_s, NULL);
        pthread_join(id1, NULL);
        pthread_join(id2, NULL);
    #endif

    #ifdef MULTI_THREAD_POOL

        #ifdef HARD_SYNC
            thpool_wait(thpool);
        #elif defined(SOFT_SYNC)
            static int counter = 0;
            counter = (counter+1)%5;
            if(counter == 0) thpool_wait(thpool);
        #else
            //Let the threads run freely without one waiting on the other
        #endif

        thpool_add_work(thpool, (void*)move_cars_w_e, NULL);
        thpool_add_work(thpool, (void*)move_cars_n_s, NULL);
    #endif
}

void update_all_lights_1s(void)
{
    int i;
    for(i=0; i<N_LIGHTS; i++) update_light_1s(&lights[i]);
}

void calculate_car_occupation(void)
{
    int    h, i, j, k, l;                                           //Loop iterators
    int    pos;                                                     //Position temp to help grabbing the right crossing lights
    int    counter;                                                 //Car counter for every block in the street
    float  temp;                                                    //Temporary float to calculate the percentage of occupation on a block
    struct street *temp_street;                                     //Street pointer to handle north/south and west/east roads

    #ifdef CBCL_CONTROL
        struct crossing temp_crossing;                              //Declares a crossing in case of being on CBCL control, this is passed as parameter to change light behavior
    #endif

    total_cars_on_map = 0;                                          //Clear the total car counter

    for(h=0; h<2; h++)                                              //Loop through north/south and west/east roads
    {
        if(h==0) temp_street = streets_w_e;                         //One direction at a time
        else     temp_street = streets_n_s;

        for(i=0; i<((h==0)?(N_STREETS_W_E):(N_STREETS_N_S));i++)    //For every street in each direction
        {
            temp_street[i].car_count = 0;                           //Clear the car count for this road (sum of all blocks)
            for(j=0; j<temp_street[i].block_count; j++)             //For every block in this road
            {
                counter = 0;
                for(k=0; k<temp_street[i].lane_count; k++)          //For every lane in this block
                {                                                   //For every slot occupied by a car, increase counter
                    for(l=0; l<SLOTS_PER_BLOCK; l++) if(temp_street[i].street_slots[k][l+j*SLOTS_PER_BLOCK] != 0) counter++;
                }

                temp_street[i].car_count += counter;                //Stacks the block counter to calculate the whole road occupation

                if(counter == (SLOTS_PER_BLOCK*temp_street[i].lane_count) - temp_street[i].lane_count)
                {
                    counter += temp_street[i].lane_count;           //The streets will never get 100% full because of the free slot per crossing, so if this is the only one empty, declare as full
                }

                temp = ((float)(counter*100))/(SLOTS_PER_BLOCK*temp_street[i].lane_count);
                if(temp > 0.0000f) temp += 0.5f;                    //After calculating the percentage of use, round up
                if(temp < 1.0f && temp > 0.0000f) temp = 1.0f;      //If it is below 1% and isn't zero, make it 1% so it is visible when plotting on the screen

                temp_street[i].occupation[j] = (int) temp;          //Cast int to simplify what is shown to the user REQ:M

                #ifdef CBCL_CONTROL                                 //If the block is above 60% capacity
                    if(( (temp_street[i].direction == WEST  || temp_street[i].direction == NORTH) && j > 0) ||                             //Every street has one block more than crossing lights, so the first iteration is taken out of the controls
                       ( (temp_street[i].direction == SOUTH || temp_street[i].direction == EAST ) && j < (temp_street[i].block_count-1)) )
                    {
                        if(temp_street[i].direction == WEST  || temp_street[i].direction == NORTH) pos = j -1;
                        else                                                                       pos = j;

                        if(temp_street[i].direction == NORTH ||                                                                            //Check the crossing lights before moving traffic
                           temp_street[i].direction == SOUTH) get_crossing(&temp_crossing, pos, temp_street[i].street_number);             //Get the corresponding crossing which will bring the...
                        else                                  get_crossing(&temp_crossing, temp_street[i].street_number, pos);             //...crossing street and both traffic lights

                        if(temp_street[i].occupation[j] > 60)                                                                              //REQ: #1 & #2 CBCL
                        {
                            reduce_red_time( temp_crossing.cross_light,
                                            (temp_street[i].direction == NORTH || temp_street[i].direction == SOUTH)?(NORTH_SOUTH):(WEST_EAST));
                        }
                    }
                #endif
            }
            total_cars_on_map += temp_street[i].car_count;          //Accumulate the sum of cars on every street
        }
    }
}

void get_street(struct street *s_in, enum street_direction dir, int street_id)
{
    if(dir == WEST || dir == EAST)
    {
        if(street_id < N_STREETS_W_E) *s_in = streets_w_e[street_id];
    }

    if(dir == NORTH || dir == SOUTH)
    {
        if(street_id < N_STREETS_N_S) *s_in = streets_n_s[street_id];
    }
}

void get_crossing(struct crossing *c_in, int st_w_e, int st_n_s)
{
    if(st_w_e < N_STREETS_W_E && st_n_s < N_STREETS_N_S) *c_in = crossings[st_w_e][st_n_s];
}

int get_total_car_count_on_map(void)
{
    return total_cars_on_map;
}

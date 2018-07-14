//External libraries
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#endif

//Local files
#include "app_definitions.h"
#include "traffic_manager.h"

//Local functions
void resize_screen();
void clear_screen();
void print_screen();

int main()
{
    #if defined(REAL_TIME_1X) || defined(REAL_TIME_10X)
        clock_t tick;
        clock_t last_tick;
        clock_t tick_100_ms;
        clock_t tick_1000_ms;
        int traffic_counter = 0;
        int plot_counter = 0;
    #endif

    #ifdef PROCESSOR_LIMITED
        clock_t tick_200ms = 0;
    #endif

    clock_t start_tick = clock();
    clock_t end_tick = 0;
    float time_diff = 0;

    clear_screen();
    resize_screen();
    init_traffic_manager();

    while(1)
    {
        #if defined(REAL_TIME_1X) || defined(REAL_TIME_10X)
            //Time-base generation
            tick  = clock();                            //Take the CPU clock count
            tick *= 1000;                               //Multiply by 1000 to have a milisecond base

            #ifdef REAL_TIME_10X                        //Fakes the clock pace by ten times, making the simulation faster
                tick *= 10;
            #endif

            tick /= CLOCKS_PER_SEC;                     //Divide the clock count by the amount of clocks per second
            if(tick >= SIM_TIME_MS) break;              //If it has reached the simulation time, finish it
            if(tick == last_tick)   continue;           //If the processor is idling at the same milisecond, get a new clock count
            last_tick = tick;                           //Retain the last attended milisecond for the next loop

            //100ms loop - loose attendance requirement
            //Note that if a 1ms time-frame gets skipped, the system will attend the 100ms event on the next milisecond
            if(tick_100_ms != (tick/100))
            {
                tick_100_ms = (tick/100);               //Save the current 10ms time-frame number to avoid oversampling

                traffic_counter = (traffic_counter+1)%2;
                if(traffic_counter == 0)
                {
                    update_traffic_200ms();
                }

                #ifdef REAL_TIME_1X
                    plot_counter = (plot_counter+1)%5;  //Update the screen every 500ms - only if real time is at 1x time-lapse
                    if(plot_counter == 0)
                    {
                        clear_screen();
                        print_screen();
                    }
                #endif
            }

            //1s loop - loose attendance requirement
            //Note that if a 1ms time-frame gets skipped, the system will attend the 1s event on the next milisecond
            if(tick_1000_ms != (tick/1000))
            {
                tick_1000_ms = (tick/1000);             //Save the current 10ms time-frame number to avoid oversampling

                #ifdef REAL_TIME_10X
                    plot_counter = (plot_counter+1)%5;  //Update the screen every 500ms note that on 10x the screen refresh is still at 500ms
                    if(plot_counter == 0)
                    {
                        clear_screen();
                        print_screen();
                    }
                #endif
            }
        #endif

        #ifdef PROCESSOR_LIMITED
            tick_200ms+=200;
            update_traffic_200ms();
            if( tick_200ms >= SIM_TIME_MS)
            {
                calculate_car_occupation();
                break;
            }
        #endif
    }

    clear_screen();
    print_screen();

    delete_din_data();

    end_tick = clock();
    time_diff = end_tick -start_tick;
    time_diff /= ((float)CLOCKS_PER_SEC);
    printf("\n");
    printf("Elapsed time: %f s", time_diff);
    getchar();

    return 0;
}

void resize_screen()                                //Changing screen size of the terminal is not a ANSI standard cmd
{                                                   //So depending on the environment, a different system msg is sent to the console
    #if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
        system("resize -s 55 140");                 //Set a constant screen size to fit 10x10 streets on the screen
    #endif

    #if defined(_WIN32) || defined(_WIN64)
        system("mode 140, 55");                     //Set a constant screen size to fit 10x10 streets on the screen

        HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO info;                   //Hide the cursor to avoid having it on the screen randomly
        info.dwSize = 100;
        info.bVisible = FALSE;
        SetConsoleCursorInfo(consoleHandle, &info);
    #endif
}

void clear_screen()                                 //Clearing the terminal is not a ANSI standard cmd
{                                                   //So depending on the environment, a different system msg is sent to the console
    #if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
        printf("\033[%d;%dH", 1, 1);                //To avoid flickering, instead of clearing the whole terminal, set the cursor to the position 0,0
    #endif

    #if defined(_WIN32) || defined(_WIN64)
        COORD Coord;                                //To avoid flickering, instead of clearing the whole terminal, set the cursor to the position 0,0
        Coord.X = 0;
        Coord.Y = 0;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Coord);
    #endif
}

void print_screen()
{
    int i;
    int j;

    struct street   temp_street;
    struct crossing temp_crossing;

    printf(" ");                                                    //Prints the direction of the north/south streets
    for(i=0; i<N_STREETS_N_S; i++)
    {
        get_street(&temp_street, NORTH, i);
        if(temp_street.direction == NORTH) printf("         x");
        else                               printf("         o");
    }
    printf("\n\n");

    for(j=0; j<=N_STREETS_W_E; j++)
    {
        printf("  ");
        for(i=0; i<N_STREETS_N_S; i++)                              //Prints the percentage of cars in the block of the north/south streets
        {
            get_street(&temp_street, NORTH, i);
            printf("       %03d", temp_street.occupation[j]);
        }
        printf("\n\n");

        if(j >= N_STREETS_W_E) break;

        get_street(&temp_street, WEST, j);
        if(temp_street.direction == WEST) printf("x");              //Prints the direction of the west/east streets
        else                              printf("o");

        for(i=0; i<=N_STREETS_N_S; i++)
        {
            printf("   %03d", temp_street.occupation[i]);           //Prints the percentage of cars in the block of the west/east streets
            if(i!=N_STREETS_N_S)
            {
                get_crossing(&temp_crossing, j, i);                 //Prints the direction in which the crossing lights are open

                if(temp_crossing.cross_light->light_n_s != RED &&
                   temp_crossing.cross_light->light_w_e == RED)
                {
                    if(temp_crossing.street_n_s->direction == NORTH) printf("   ^");
                    else                                             printf("   v");
                }
                else if(temp_crossing.cross_light->light_n_s == RED &&
                        temp_crossing.cross_light->light_w_e != RED)
                {
                    if(temp_crossing.street_w_e->direction == WEST) printf("   <");
                    else                                            printf("   >");

                }
                else printf("   +");                                //This means both sides are closed, aka dead-time
            }
        }

        if(temp_street.direction == WEST)  printf("   o");          //Prints the direction of the west/east streets
        else                               printf("   x");
        printf("  %1d",temp_street.lane_count);                     //Prints the amount of lanes for this street
        printf("  %04d", temp_street.car_count);                    //Prints the amount of cars in the current street
        printf("\n\n");
    }

    printf(" ");
    for(i=0; i <N_STREETS_N_S; i++)
    {
        get_street(&temp_street, NORTH, i);
        if(temp_street.direction == NORTH) printf("         o");    //Prints the direction of the north/south streets
        else                               printf("         x");
    }
    printf("\n\n");

    printf(" ");
    for(i=0; i <N_STREETS_N_S; i++)
    {
        get_street(&temp_street, NORTH, i);
        printf("         %1d",temp_street.lane_count);              //Prints the amount of lanes for this street
    }
    printf("\n\n");

    printf("  ");
    for(i=0; i <N_STREETS_N_S; i++)
    {
        get_street(&temp_street, NORTH, i);
        printf("      %04d", temp_street.car_count);                //Prints the amount of cars in the current street
    }
    printf("\n\n");                                                 //Prints some final statistics
    printf("Total cars on map: %6d", get_total_car_count_on_map());
    printf("\n");
    printf("Cars passed on map:%6d", get_cars_passed_on_map());
}

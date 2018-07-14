#ifndef __CAR_H__
    #define __CAR_H__

    //Locals
    #include "app_definitions.h"

    #ifndef CAR_SIZE                    //Car size in meters
        #error no CAR_SIZE definition
    #endif                              // CAR_SIZE

    struct car
    {
        unsigned char   on_map;
        unsigned int    id;
    };


#endif                                  // __CAR_H__

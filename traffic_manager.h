#ifndef __TRAFFIC_MANAGER_H__
    #define __TRAFFIC_MANAGER_H__

    //External libs
    #include <stdlib.h>
    #include <time.h>

    //Local files
    #include "app_definitions.h"
    #include "street.h"

    //Local functions
    void init_traffic_manager(void);
    void update_traffic_200ms(void);
    int  get_cars_passed_on_map(void);
    void delete_din_data(void);

#endif

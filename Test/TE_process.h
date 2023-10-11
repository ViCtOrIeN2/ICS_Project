#ifndef TE_PROCESS_H
#define TE_PROCESS_H


#include <math.h>
#include <lapacke.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <jsoncpp/json/json.h>



//TODO add sensor noise?

class TE {
        struct timeval last_update, current; 
        double time_scale;
        double temperature;
        int thermostat_mode;

    public:
        TE();
        void update(Json::Value inputs);
        void print_outputs();
        Json::Value get_state_json() ;
        
};



#endif

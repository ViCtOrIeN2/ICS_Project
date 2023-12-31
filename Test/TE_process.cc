#include "TE_process.h"

TE::TE() {
    gettimeofday(&current, NULL);
    last_update = current;
    temperature = 80.0;
    thermostat_mode = 0;
}

/*void TE::steady_state() {
    lapack_int n, nrhs, lda, ldb, info;
    int i, j;
    double *A, *b;
    lapack_int *ipiv;
    
    //matrix inits
    n = 3; nrhs = 1;
    lda=n, ldb=nrhs;

    double Pa = pressure*A_in_purge;
    double Pc = pow(product_flow/(kpar*(pow(Pa,1.2))),(1/ncpar));
    C_in_purge = Pc/pressure;
    double Pb = pressure - Pa - Pc;
    B_in_purge = Pb/pressure;
    yc1 = 1 - ya1 - yb1;
    
    
    //matrix inits
    A = (double *)malloc(n*n*sizeof(double)) ;
    if (A==NULL){ printf("error of memory allocation\n"); exit(0); }
    b = (double *)malloc(n*nrhs*sizeof(double)) ;
    if (b==NULL){ printf("error of memory allocation\n"); exit(0); }
    ipiv = (lapack_int *)malloc(n*sizeof(lapack_int)) ;
    if (ipiv==NULL){ printf("error of memory allocation\n"); exit(0); }
    
    //lapack wants the matrix in row major form
    A[0] = ya1;
    A[1] = 1;
    A[2] = -A_in_purge;
    A[3] = yb1;
    A[4] = 0;
    A[5] = -B_in_purge;
    A[6] = yc1;
    A[7] = 0;
    A[8] = -C_in_purge;
 
    b[0] = product_flow;
    b[1] = 0;
    b[2] = product_flow;
    
    
    // Solve the equations A*X = B 
    info = LAPACKE_dgesv( LAPACK_ROW_MAJOR, n, nrhs, A, lda, ipiv,
                         b, ldb );
    // Check for the exact singularity 
    if( info > 0 ) {
        printf( "The diagonal element of the triangular factor of A,\n" );
        printf( "U(%i,%i) is zero, so that A is singular;\n", info, info );
        printf( "the solution could not be computed.\n" );
        exit( 1 );
    }
    if (info <0) exit( 1 );
    
    f1_valve_pos = b[0]/cv[0];
    f2_valve_pos = b[1]/cv[1];
    purge_valve_pos = b[3]/(cv[2]*sqrt(pressure-100));
    product_valve_pos = product_flow/(cv[3]*sqrt(pressure-100));
    
    molar_D = 110;
    liquid_level = molar_D/8.3;
    double VV = VT-liquid_level;
    double NV = pressure*VV/(8.314*373);
    molar_A = NV*A_in_purge;
    molar_B = NV*B_in_purge;
    molar_C = NV*C_in_purge;
} 
*/

void TE::update(Json::Value inputs) {
    //TODO limit valve positions, water level, and pressure 
    // TODO only modify given inputs
    double temperature = temperature;
    double thermostat_mode = thermostat_mode;
    if (inputs["inputs"].isMember("temperature")) {
        temperature = inputs["inputs"]["temperature"].asDouble();
    }
    if (inputs["inputs"].isMember("thermostat_mode")) {
        thermostat_mode = inputs["inputs"]["thermostat_mode"].asInt();
    }
    /*double f1_valve_sp = f1_valve_pos;
    double f2_valve_sp = f2_valve_pos;
    double purge_valve_sp = purge_valve_pos;
    double product_valve_sp = product_valve_pos;
    if (inputs["inputs"].isMember("f1_valve_sp")) {
        f1_valve_sp = inputs["inputs"]["f1_valve_sp"].asDouble();
    }
    if (inputs["inputs"].isMember("f2_valve_sp")) {
        f2_valve_sp = inputs["inputs"]["f2_valve_sp"].asDouble();
    }
    if (inputs["inputs"].isMember("purge_valve_sp")) {
        purge_valve_sp = inputs["inputs"]["purge_valve_sp"].asDouble();
    }
    if (inputs["inputs"].isMember("product_valve_sp")) {
        product_valve_sp = inputs["inputs"]["product_valve_sp"].asDouble();
    }

    last_update = current;
    gettimeofday(&current, NULL);
    double dt = ((current.tv_sec - last_update.tv_sec) + (current.tv_usec - last_update.tv_usec)/1000000.0)/60/60; //measurements are in hours
    dt = dt*time_scale;
    f1_valve_pos = f1_valve_sp;
    f2_valve_pos = f2_valve_sp;
    purge_valve_pos = purge_valve_sp;
    product_valve_pos = product_valve_sp;
    // TODO double check to make sure I didn't skip anything
    double NL = molar_D;                        // total liquid moles [kmol]
    double VL = NL/Lden;                        // liquid volume [m^3]
    double VV = VT-VL;                          // vapor volume [m^3]
    yc1 = 1.0 - ya1- yb1;                // mole fraction C in feed 1
    double NV = molar_A + molar_B + molar_C;    // total vapor moles [kmol]
    pressure = NV * Rgas * Tgas / VV;          // total pressure [kPa]
    liquid_level = VL * 100.0 / VLmax;          // liquid volume as percentage of capacity
    
    //flow rates
    f1_flow = cv[0] * f1_valve_pos;
    f2_flow = cv[1] * f2_valve_pos;
    if (pressure >= 100.0) {
        purge_flow = cv[2] * purge_valve_pos * sqrt(pressure-100.0);  
        product_flow = cv[3] * product_valve_pos * sqrt(pressure-100.0);
    } else {
        purge_flow = 0.0;
        product_flow = 0.0;
    }
    
    if (product_valve_pos <= 0.0) {
        product_flow = 0.0;
    }
    // if liquid level is 0 then product flow must be 0
    if (liquid_level <= 0.0) {
        product_flow = 0.0;
    }
    
    
    // vapor composition
    A_in_purge = molar_A / NV;
    B_in_purge = molar_B / NV;
    C_in_purge = molar_C / NV;
    
    // check for  delayed sampling
    if (current.tv_sec*1000000 +current.tv_usec > tpurge) {
        tpurge = tpurge + int(sampling_delay*60*60*1000000);
        printf("********************************************\n");
        for (int i=0; i++; i<3) {
            ymeas[i] = ylast[i];
        }
        ylast[0] = A_in_purge;
        ylast[1] = B_in_purge;
        ylast[2] = C_in_purge;
    }
    
    
    // reaction rate parameters
    // if 0 use defaults
    if (kpar <= 0.0) {
        kpar = 0.00117;
    }
    if (ncpar <= 0.0) {
        ncpar = 0.4;
    }
    
    double Pa = A_in_purge * pressure;
    double Pc = C_in_purge * pressure;
    double RR1 = kpar * pow(Pa,1.2)*pow(Pc,ncpar);
    

    dxdt_molar_A = ya1*f1_flow + f2_flow - A_in_purge*purge_flow - RR1;
    dxdt_molar_B = yb1*f1_flow - B_in_purge*purge_flow;
    dxdt_molar_C = yc1*f1_flow - C_in_purge*purge_flow - RR1;
    dxdt_molar_D = RR1 - product_flow;
    
    
    
    
    
    molar_A+= dxdt_molar_A * dt;
    molar_B+= dxdt_molar_B * dt;
    molar_C+= dxdt_molar_C * dt;
    molar_D+= dxdt_molar_D * dt;
        
    
    // check physical limits on all variables TODO move in better place
    f1_valve_pos = std::max(std::min(f1_valve_pos, 100.0), 0.0);
    f2_valve_pos = std::max(std::min(f2_valve_pos, 100.0), 0.0);
    purge_valve_pos = std::max(std::min(purge_valve_pos, 100.0), 0.0);
    product_valve_pos = std::max(std::min(product_valve_pos, 100.0), 0.0);
    liquid_level = std::max(std::min(liquid_level, 100.0), 0.0);
    pressure = std::max(std::min(pressure, 3200.0), 0.0);
    
    molar_A = std::max(molar_A, 0.0);
    molar_B = std::max(molar_B, 0.0);
    molar_C = std::max(molar_C, 0.0);
    molar_D = std::max(molar_D, 0.0);

    
    
    
    if (product_flow <= 0.0) {
        //hold at last cost
        cost = cost;
    } else {
        cost = purge_flow * (ymeas[0]*2.206 + ymeas[2]*6.177)/product_flow; // cost/mole product
    }
    */
}

void TE::print_outputs() {
    printf("\n\ntemperature= %.2f", temperature);
    printf("\nF1thermostat mode = %d", thermostat_mode);
    
}


Json::Value TE::get_state_json() {
    Json::Value state;
    state["process"] = "simpleTE";
    state["outputs"]["temperature"] = temperature;
    state["states"]["thermostat_mode"] = thermostat_mode;
    
    
    
    
    return state;

}




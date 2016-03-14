

#include "parallel_scheduler.h"

#include <iostream>
#include <random>
#include "utils.h"
#include "simulation.h"
#include "parameters_struct.h"

int main(int argc, char *argv[]){

    //MPI OPEN
    /********************************************/
    MPI::Init();

    PPS::comm_size =  MPI::COMM_WORLD.Get_size(); //Set global variable
    PPS::pid = MPI::COMM_WORLD.Get_rank(); //Set global variable
    /********************************************/

    double t1,t2;
    int sims, division, rem, p, threads=10; //count the total sims

    if(PPS::pid == 0){
        int i,j,k;

        t1 = MPI::Wtime();
        std::cout << "Potts simulation" << std::endl;
        struct parameters params;
        load_params("params.cfg", params);


        int S[] = {3, 5, 7, 9};
        for(i = 0; i < 4; ++i){
            params.S = S[i];
            for(j = 10; j < 1000; j+=50 ){

                params.p = j;

                p = params.p;
                if(p > 100) p = 100;

                division = p / threads; //Number of params over number of threads
                rem = p % threads;

                for(k = 0; k < division; ++k){
                    params.start_cue = threads*k;
                    params.end_cue = params.start_cue + threads;
                    PPS::plist.push_back(params);
                }
                //Last sim in case there's a remainder
                if(rem != 0){
                    params.start_cue = threads*division;
                    params.end_cue = params.start_cue + rem;
                    PPS::plist.push_back(params);
                }

            }
        }

        sims = PPS::plist.size();

    }



    PPS::start();

    if(PPS::pid == 0){
        t2 = MPI::Wtime();
        std::cout << "[NSIM] [ELAPSED TIME] " << sims << " " << t2-t1 << std::endl;
        std::cout << "End of the simulation" << std::endl;
    }

    //MPI CLOSE
    /**************************************************************************/
    MPI::Finalize();
    /**************************************************************************/


    return 0;
}

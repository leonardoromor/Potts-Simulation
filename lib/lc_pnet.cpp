#include <math.h>
#include <iostream>
#include <random>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <string>
#include <sstream>
#include <chrono>

#include "lc_pnet.h"
#include "config.h"
#include "random_seq.h"


/*******************************************************************************
HIGH CONNECTIVITY POTTS NETWORK
*******************************************************************************/
LC_PNet::LC_PNet(const int & N, const int & C, const int & S) : PNet(N){

    int i,j;
    this->C = C;
    this->S = S;
    this->cm = new int[N * C];
    this->ucm = new int[N * N];
    this->J = new __fpv[N * S * C * S]; //!"£"
    this->inactive_states = new __fpv[N];
    this->active_states = new __fpv[N * S];
    this->inactive_r = new __fpv[N];
    this->active_r = new __fpv[N * S];
    this->h = new __fpv[N*S];
    this->theta = new __fpv[N*S];

    //Init to 0 "1-0 matrix"
    for(i = 0; i < N; ++i){
        for(j = 0; j < N; ++j){
            ucm[i*N + j] = 0;
        }
    }

}

LC_PNet::~LC_PNet(){

    delete[] this->cm;
    delete[] this->ucm;
    delete[] this->J;
    delete[] this->inactive_states;
    delete[] this->active_states;
    delete[] this->inactive_r;
    delete[] this->active_r;
    delete[] this->h;
    delete[] this->theta;

}

void LC_PNet::connect_units(std::default_random_engine & generator){

    int i,j;
    RandomSequence sequence(this->N); //Sequence between 0 and N-1

    //Fill cm matrix with indices of potts units
    for(i = 0; i < this->N; ++i){
        //Shuffle the sequence
        sequence.shuffle(generator);

        //Store in the inverse cm
        for(j = 0; j < C; ++j){
            this->cm[C*i + j] = sequence.begin()[j];
            this->ucm[i*N + sequence.begin()[j]] = 1;
        }
    }

}



void LC_PNet::init_states(const __fpv & beta, const __fpv & U){

    int i,j;

    double n = -2 * beta - 2 * exp(beta * U) - 2 * S+sqrt(pow(2 * beta + 2 * exp(beta * U)+2 * S,2)+8 * (-beta * beta - 2 * beta * S + 2 * beta *S * exp(beta * U)));
    n /= 2 * (-beta * beta - 2 * beta * S + 2 * beta * S * exp(beta * U));

    for(i = 0; i < this->N; ++i){

        for(j = 0; j < this->S; ++j){
            this->active_states[i * S + j] = n;
        }

        this->inactive_states[i] = 1 - this->S*this->active_states[i * S];
        this->inactive_r[i] = 1 - this->inactive_states[i];
    }
}
void LC_PNet::init_J(const int & p, const __fpv & a, const int * xi){

    int i,j,k,l,m;
    __fpv as = a/S;

    //Generate Jkxl
    for(i = 0; i < N; ++i){
        for(j = 0; j < S; ++j){

            this->h[S*i + j] = 0;

            for(k = 0; k < C; ++k){
                for(l = 0; l < S; ++l){

                    this->J[S*C*S*i + C*S*j + S*k + l] = 0;

                    for(m = 0; m < p; ++m){
                        this->J[S*C*S*i + C*S*j + S*k + l] += ((xi[p * i + m]==j)-as)*((xi[p * cm[C*i + k] + m]==l)-as);
                    }

                    this->J[S*C*S*i + C*S*j + S*k + l] /= (__fpv)(a * (1.0 - as)*(__fpv)C);

                    this->h[S*i + j] += this->J[S*C*S*i + C*S*j + S*k + l] * this->active_states[S*cm[C*i + k] + l];

                }
            }

            this->active_r[S*i + j] = this->h[S*i + j];
            this->theta[S*i + j] = this->active_states[S*i + j];

        }
    }


}

void LC_PNet::init_network(const __fpv & beta, const __fpv & U, const int & p, const __fpv & a, const int * xi){

        //Init states
        this->init_states(beta,U);

        //Init J
        this->init_J(p,a,xi);


}

void LC_PNet::update_rule(const int & unit, const __fpv buffer[], const int & pattern, const __fpv & U, const __fpv & w, const __fpv & g, const __fpv & tau, const __fpv & b1, const __fpv & b2, const __fpv & b3, const __fpv & beta, const int & tx, const int & t){

    //tx == n0 in the old code, "time 'x' "
    int i,j;
    __fpv self=0, INcost, rmax, Z;
    int tsize = this->C * this->S;

    //Second optimization START
    const __fpv tb3 = b3;
    const __fpv tb1 = b1;
    const __fpv tb2 = b2;
    const __fpv tbeta = beta;

    //Temp variables
    __fpv temp;
    __fpv * Jt = this->J + S*C*S*unit;

    double e1[this->S];
    //Second optimization END


    rmax = this->inactive_r[unit];

    for(i = 0; i < this->S; ++i){
        self += this->active_states[unit*S + i];
        this->h[unit*S + i] = 0; //Maybe h can be even a small array of size S, since it's erased at each update
    }

    self = (w / this->S) * self;

    INcost = (t > tx) * g * exp(-(t-tx)/tau);

    for(i = 0; i < this->S; ++i){

        //Inside here maybe different order of + and * so slightly different solutions, have to check.
        //#pragma novector
        //__assume_aligned(&buffer, 64);
        //__assume_aligned(&Jt, 64);

        temp = 0;


        //#pragma vector aligned
        //#pragma novector
        //#pragma vector always
        for(j = 0; j < tsize; ++j){
            temp += *(Jt++) * buffer[j];
            //std::cout << temp << std::endl;
        }

        // int tmp = S*C*S*unit + C*S*i;
        // Jt = this->J + tmp;
        // const int ONE = 1;
        // float temp = sdot(&tsize, Jt, &ONE, buffer, &ONE);


        // for(j = 0; j < tsize; ++j){
        //     this->h[unit*S + i] += this->J[S*C*S*unit + C*S*i + j] * buffer[j];
        // }

        this->h[unit*S + i] += temp + (w * this->active_states[unit*S + i] - self + INcost * (pattern == i));

        this->theta[unit*S + i] += tb2 * (this->active_states[unit*S + i]-this->theta[unit*S + i]);
	    this->active_r[unit*S + i] += tb1 * (this->h[unit*S + i]-this->theta[unit*S + i]-this->active_r[unit*S + i]);

        rmax = this->active_r[unit*S + i] * (this->active_r[unit*S + i] > rmax) - ((this->active_r[unit*S + i] > rmax) - 1) * rmax;

        /*
        if(this->active_r[unit*S + i]>rmax){
            rmax=this->active_r[unit*S + i];
	    }
        */


    }

    this->inactive_r[unit] += tb3 * (1.0 - this->inactive_states[unit] - this->inactive_r[unit]);

    /*
    VERSION 1.0 "last part", just a copy of the old code
    */
    // Z=0;
    // for(i = 0; i < S; ++i){
    //     Z += exp(tbeta * (this->active_r[unit*S + i] - rmax));
    // }
    // Z += exp(beta * (this->inactive_r[unit] + U - rmax));
    // Z = 1.0/Z;
    // for(i = 0; i < S; ++i){
    // 	this->active_states[unit*S + i] = exp(tbeta * (this->active_r[unit*S + i] - rmax)) * Z;
    // }
    // this->inactive_states[unit]=exp(beta * (this->inactive_r[unit] - rmax + U)) * Z;
    /*
    END OF VERSION 1.0 "last part", just a copy of the old code
    */


    /*
    VERSION 2.0 "last part"
    */
    Z = 0;
    for(i = 0; i < S; ++i){
        e1[i]= exp(tbeta * (this->active_r[unit*S + i] - rmax));
        Z += e1[i];
    }

    Z += exp(tbeta * (this->inactive_r[unit] + U - rmax));
    Z=1.0/Z;

    for(i = 0; i < S; ++i){
        this->active_states[unit*S + i] = e1[i] * Z;
    }
    //Just changing the rmax and U sum order change for some configurations the output of the simulation.
    //that it could go faster without reevaluating exp(tbeta * (this->inactive_r[unit] + U - rmax))
    this->inactive_states[unit] = exp(beta * (this->inactive_r[unit] - rmax + U))*Z;
    /*
    END OF VERSION 2.0 "last part"
    */
}

void LC_PNet::start_dynamics(std::default_random_engine & generator, const int & p,const int & tstatus, const int & nupdates, const int * xi, const int & pattern, const __fpv & a, const __fpv & U, const __fpv & w, const __fpv & g, const __fpv & tau, const __fpv & b1, const __fpv & b2, const __fpv & b3, const __fpv & beta, const int & tx){

    //The code here is wrote for different cases defined during preprocessor
    int i,j,k,n,t;
    int unit;
    __fpv buffer[C * S] __attribute__((aligned(64)));

    RandomSequence sequence(this->N);

    this->latching_length = 0;
    bool stop = false;
    int Mumax = p + 5, Mumaxold = p + 5, steps = 0;

    const __fpv tb3 = b3;
    const __fpv tb1 = b1;
    const __fpv tb2 = b2;
    const __fpv tbeta = beta;

    t = 0;

    //First loop = times the whole network has to be updated
    for(i = 0; i < nupdates; ++i){

        //Shuffle the random sequence
        #ifndef _NO_SHUFFLE
        sequence.shuffle(generator);
        #endif

        //Second loop = loop on all neurons serially
        for(j = 0; j < N; ++j){

            unit = sequence.get(j);

            //__assume_aligned(&buffer, 64);

            //Fill the buffer containing all the states requested
            for(k = 0; k < this->C; ++k){
                for(n = 0; n < this->S; ++n){
                    buffer[k*S + n] = this->active_states[S*cm[unit * C + k] + n];
                }
            }

            //Update the unit
            this->update_rule(unit,
                             buffer,
                             xi[p * unit + pattern],
                             U,
                             w,
                             g,
                             tau,
                             tb1,
                             tb2,
                             tb3,
                             tbeta,
                             tx,
                             t
                             );


            if((t % tstatus) == 0){
                latching_length = (double)t / N;
                this->get_status(p,tx,t,xi,a,Mumaxold,Mumax,steps,stop);

                #ifndef _NO_END_CONDITION
                if(stop &&  (t > tx + 100 * N)) goto end;
                #endif
            }

            t++;

        }

    }
    #ifndef _NO_END_CONDITION
    end:
    #endif

    #ifdef _TEST
    if(t > tx + 100 * N){

        std::cout << "Latching length: " <<  latching_length << std::endl;
    }else{
        std::cout << "Simulation finished before reaching minimum steps (" << (double)t / N << ")" << std::endl;
    }
    #endif
    std::cout;

}

void LC_PNet::evaluate_m(const int & p, const __fpv & a, const int * xi, __fpv m[]){
    int i,j,k;

    __fpv ma, maa;
    __fpv invdenN = 1/(a*(1-a/S)*N);

    for(i = 0; i < p; ++i){
        maa = 0;
        for(j = 0; j < N; ++j){
            ma = 0;
            for(k = 0; k < S; ++k){
                ma += ( (xi[p * j + i] == k) - a/S) * this->active_states[S*j + k];
            }
            maa += ma;
        }
        m[i] = maa*invdenN;

    }

}

void LC_PNet::print_cm(){

    int i,j;

    for(i=0; i < this->N; i++){
        for(j=0; j < this->C; ++j){
            std::cout << this->cm[this->C*i + j]<< " ";
        }
        std::cout << std::endl;
    }

}

void LC_PNet::save_connections_to_file(const std::string & filename){

    std::ofstream ofile;
    int i,j;
    ofile.open(filename);

    for(i = 0; i < this->N; i++){
        for(j= 0; j < this->C; j++){
            ofile << *(this->cm + this->C*i + j)<< " ";
        }
        ofile << std::endl;
    }
    ofile.close();

}

void LC_PNet::save_states_to_file(const std::string & filename){

    std::ofstream ofile;
    int i,j;
    ofile.open(filename);
    ofile.precision(15);
    ofile << std::scientific;

    for(i = 0; i < this->N; i++){
        for(j= 0; j < this->S; j++){
            ofile << this->active_states[S*i + j]<< " ";
        }
        ofile << this->inactive_states[i]<< " ";
        ofile << std::endl;
    }
    ofile.close();

}

void LC_PNet::save_J_to_file(const std::string & filename){

    std::ofstream ofile;
    int i,j,k,l;
    ofile.open(filename);
    ofile.precision(15);
    ofile << std::scientific;

    for(i = 0; i < this->N; ++i){
        for(j = 0; j < this->S; ++j){
            for(k = 0; k < this->C; ++k){
                for(l = 0; l < this->S; ++l){
                    ofile << this->J[S*C*S*i + C*S*j + S*k + l] << " ";
                }
            }
        }
        ofile << std::endl;
    }

    ofile.close();
}

void LC_PNet::import_connections(const std::string & filename){

    int i, j;
    std::ifstream ifile;
    ifile.open(filename);

    for(i = 0; i < this->N; ++i){
    	for(j = 0; j < this->C; ++j){
    		ifile >> this->cm[this->C*i + j];
    	}

    }

    ifile.close();
}

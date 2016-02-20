#include <iostream>
#include <random>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <cstdlib>
#include <time.h>
#include "config.h"
#include "pattern_gen.h"

PatternGen::PatternGen(int N, int p, int S, __fpv a, __fpv beta, int N_fact, int Num_fact, __fpv a_fact, __fpv eps, __fpv a_pf, __fpv fact_eigen_slope){

    this->N = N;
    this->p = p;
    this->S = S;
    this->a = a; //Sparsity parameter
    this->beta = beta;
    this->N_fact = N_fact;
    this->Num_fact = Num_fact;
    this->a_fact = a_fact;
    this->eps = eps;
    this->a_pf = a_pf;
    this->fact_eigen_slope = fact_eigen_slope;

    this->Patt = new int[N * p];

}

PatternGen::~PatternGen(){
    delete[] this->Patt;
}

void PatternGen::generate(){

    int N_p,i,j,k,m,s1,unit;
    __fpv y, h_max, eigen_fact, sum_e, piccolo, a_pa,  a_patt, dh, h000,expon,fluct,bb;
    __fpv hh[this->N][this->S],hhh[this->S],ss[this->S];
    int Factors[N_fact][Num_fact];

    std::uniform_int_distribution<int> int_uniform_0_Nm1(0,this->N-1);

    srand48( 12345 );
    //srand48( time(NULL) );

    //Set factors
    for(i=0; i<this->Num_fact; i++){
        for(j=0; j<this->N_fact; j++){
            Factors[j][i] = (int)((double)this->N*drand48());//int_uniform_0_Nm1(generator);
        }
    }


    //Set patterns
    piccolo = log(this->eps);
    a_patt = 0.0;

    for(m=0; m<p; m++){

        //Init hh to 0
        for(j=0; j<this->N; j++){
            for(k=0; k< this->S; k++){

                hh[j][k] = 0.0;

            }
        }

        k = 0;
        expon = 0.0;


        while(expon > piccolo){

            expon = -fact_eigen_slope * k; //gamma in the paper

            if((k+2) > this->Num_fact){
                expon = 2.*piccolo;
            }

            y = drand48();

            if(y <= this->a_pf){

                eigen_fact = exp(expon)*y/this->a_pf; //a_pf is a in the paper
                s1 = (int)((float)this->S*drand48());

                for(j=0; j<this->N_fact; j++){
                    hh[Factors[j][k]][s1] += eigen_fact + this->eps * drand48();
                }
            }

            k++;
        }


        a_pa = 0.0;
        h000 = this->Num_fact;
        k = 0;

        if(this->fact_eigen_slope > (1.0/this->Num_fact)){
            h000 = 1./this->fact_eigen_slope;
        }

        h000 *= (0.5 * this->a_pf * this->a_fact)/this->S;
        bb = this->beta;

        while(((a_pa - this->a)*(a_pa - this->a)) >= this->eps){

            N_p = 0;
            fluct = sqrt(this->eps)*k;


            for(unit=0;unit<this->N;unit++){

                h_max = 0.0;

                for(s1=0; s1<this->S; s1++){
                    hhh[s1] = hh[unit][s1] + fluct*drand48();//std::rand()/RAND_MAX;//real_uniform_0_1(*this->generator);
                    if(hhh[s1]>h_max)h_max = hhh[s1];
                }

                sum_e = 0.0;

                for(s1=0;s1<this->S;s1++){
                    dh = hhh[s1] - h_max;
                    if(dh>piccolo)sum_e += exp(bb*dh);
                }

                dh = h000 -h_max;

                if(dh>piccolo){
                    sum_e += exp(bb*dh);
                }

                Patt[unit*this->p + m] = this->S;

                for(s1=0;s1<this->S;s1++){

                    ss[s1] = exp(bb*(hhh[s1]-h_max))/sum_e;

                    if(ss[s1]>=0.5){
                        N_p++;
                        Patt[unit*this->p + m] = s1;
                    }
                }

            }

            a_pa = (float)N_p/this->N;
            h000 += 0.1*(a_pa - this->a);

            k++;

        }

       a_patt += a_pa/this->p;

    }

    std::cout << "Average pattern sparsity=" << a_patt << std::endl;

}

void PatternGen::eval_stats(){

    int mu, nu, i;
    __fpv C[p][p], coppie = 0, media = 0, varianza = 0, mC, vC;

    //Evaluating correlation
    for(mu=0;mu<p;mu++){
        for(nu=0;nu<p;nu++){
            C[mu][nu]=0.0;
            for(i=0;i<N;i++){
                C[mu][nu]+= (Patt[i*this->p + mu]==Patt[i*this->p + nu]);
            }
            C[mu][nu]=C[mu][nu]/N;
        }

    }

    //Mean and standard deviation
    for(mu=0;mu<(p-1);mu++){
        for(nu=(mu+1);nu<p;nu++){
            media+= C[mu][nu];
            coppie++;
        }

    }

    mC=media/coppie;

    for(mu=0;mu<(p-1);mu++){
        for(nu=(mu+1);nu<p;nu++){
        varianza+= C[mu][nu]* C[mu][nu]-mC*mC;
        }
    }

    vC=varianza/coppie;

    //Display output
    std::cout << "<C>: Mean="
              << std::setprecision( 4 )
              << mC
              << "  Standard Dev="
              << sqrt(vC)
              << std::endl;

}

void PatternGen::save_pattern_to_file(std::string filename){

    std::ofstream ofile;
    int i,j;
    ofile.open(filename);

    for(i = 0; i < this->p; i++){
        for(j= 0; j < this->N; j++){
            ofile << this->Patt[j*this->p + i]<< " ";
        }
        ofile << std::endl;
    }
    ofile.close();
}

int * PatternGen::get_patt(){
    return this->Patt;
}

int * PatternGen::get_patt(const int n){
    return (this->Patt + n*this->p);
}

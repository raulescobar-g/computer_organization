#include <iostream>
#include <thread>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include "Semaphore.h"
using namespace std;

#define A 100                               // threads to run per letter
#define B 100
#define C 100

int calculation = 0;                        // dummy counter incremented by each thread running

Semaphore m(1);
Semaphore a(1);
Semaphore b(0);
Semaphore c(0);
int bdone = 0;

void thread_a(){
    while (true){
        a.P();                                              // we say that thread a has begun executing

        m.P();
        ++calculation;
        cout<<endl;
        cout<<"Cycle: "<<calculation/4 + 1<<endl;
        cout<<"============================================="<<endl;
        cout<<"thread A locked and executing..."<<calculation<<endl;     // thread a does some operation
        m.V();

        b.V();                                              // we give 2 threads b the chance to run when we finish
        b.V();
    }
}
void thread_b(){
    while(true){
        b.P();                                              // we let b begin execution
        
        m.P();
        ++calculation;
        cout<<"thread B locked and executing..."<<calculation<<endl;
        m.V();
        
        m.P();
        ++bdone;
        if (bdone == 2){                                    // we let the last b thread to allow 1 thread c to run
            c.V();
            bdone = 0;                                      // reset this variable
        }
        m.V();
    }
}
void thread_c(){
    while (true){
        c.P();                                      // almost identical to a thread

        m.P();
        ++calculation;
        cout<<"thread C locked and executing..."<<calculation<<endl;
        cout<<"============================================="<<endl;
        m.V();
        usleep (500000);                        // small pause for the grader to see the output

        a.V();                                      // restart the cycle of threads
    }
}

int main(){

    vector<thread> a_threads;
    vector<thread> b_threads;
    vector<thread> c_threads;

    for (int i = 0; i < A; ++i){
        a_threads.push_back(thread(thread_a));
    }
    for (int i = 0; i < B; ++i){
        b_threads.push_back(thread(thread_b));
    }
    for (int i = 0; i < C; ++i){
        c_threads.push_back(thread(thread_c));
    }

    for (int i =0 ;i < A; ++i){
        a_threads[i].join();
    }
    for (int i =0 ; i < B; ++i){
        b_threads[i].join();
    }
    for (int i=0; i<C; ++i){
        c_threads[i].join();
    }







    return 0;
}
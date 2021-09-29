#include <iostream>
#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<unistd.h>
using namespace std;

int main(){
    int cid1 = fork();
    wait(0);
    cout << "ID=" << getpid () << endl;
    int cid2 = fork();
    wait(0);
    cout << "ID=" << getpid () << endl;
    int cid3 = fork();
    cout << "ID=" << getpid () << endl;
    int cid4 = fork();
    cout << "ID=" << getpid () << endl;

    // for (int i=0; i<4; i++){
    //     int cid = fork ();
    //     if (i < 2){
    //         wait (0);
    //     }
    //     cout << "ID=" << getpid () << endl;
    // }
    return 0;
}
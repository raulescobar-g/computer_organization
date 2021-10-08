#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <stack>
#include <time.h>
#include "shell.h"
using namespace std;

#define PATH_MAX 1024


int main() {
    int fd;
    if (0 > (fd = open("a" , O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))){ 
                cout<<"here1"<<endl;
                perror("open");
                exit(1);
            }
    cout<<"oppenned fine"<<endl;


    return 0;
}
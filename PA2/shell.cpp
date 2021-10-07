#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <time.h>
#include "shell.h"
using namespace std;
/*
double quotes special characters stuff
username
extra points stuff
*/

// got this from stackoverflow
const std::string now() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return buf;
}

int Shell::execute(string pipes) {
    bool file_io = false;
    int fd;
    vector<string> parts = split(pipes); //words
    string slice;

    for (int i = 0; i < parts.size(); ++i){
        if (trim(parts[i]) == "<"){
            file_io = true;

            if (0 > (fd = open(parts[i+1].c_str() , O_RDONLY))){ 
                perror("open");
                exit(1);
            }
            dup2(fd,0);
             
            execute(trim(slice));

            slice = "";
        }
        else if (trim(parts[i]) == ">"){
            file_io = true;

            if (0 > (fd = open(parts[i+1].c_str() , O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))){ 
                perror("open");
                exit(1);
            }
            dup2(fd,1);
             
            execute(trim(slice));
            
            slice = "";
        }
        slice += parts[i] + " ";
    }

    if (file_io){
        return 0;
    }
    else {
        char** args = vec_to_char_array(parts);
        execvp(args[0],args);
        return 0;
    }
    
    return 1;
}

void Shell::execute_cmd(const string& inputline, bool bg){
    pipes = split(inputline, "|");

    for (int i = 0; i < pipes.size(); ++i) {
        vector<string> parts = split(pipes[i]);
    
        if (parts[0] == "cd"){
            
            if (parts[1] == "-"){
                if (paths.size() >= 1){
                    chdir(paths[paths.size() - 1].c_str());
                    paths.pop_back();
                }
                else {
                    cout<<"cd stack empty."<<endl;
                }
            }
            else {
                char cwd[PATH_MAX];
                getcwd(cwd, sizeof(cwd));
                paths.push_back(cwd);
                
                chdir(parts[1].c_str());
            }
        }
        else{
            int fd[2];
            pipe(fd);
            int pid = fork();
            if (pid == 0) {

                if (i < pipes.size() -1){
                    dup2(fd[1], 1);
                }

                int result = execute(trim(pipes[i]));
                
            }
            else {
                if (!bg){
                    waitpid(pid,0,0);
                    dup2(fd[0], 0);
                    close(fd[1]);
                }
                else {
                    bgs.push_back(pid);
                }
            }
        }
    }
}

void Shell::kill_idle_children(){
    for (int i = 0 ; i < bgs.size(); i++){
        if (waitpid(bgs[i],0,WNOHANG) == bgs[i]){
            cout<< "Process: "<<bgs[i]<<" ended"<<endl;
            bgs.erase(bgs.begin()+i);
            i--;
        }
    }
}

string Shell::get_cmd_prompt(){
    string inputline;
    int user;
    char username[64];

    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    if ( 0 > (user = getlogin_r(username, sizeof(username)))){
        perror("username");
        exit(1);
    }

    std::cout<<now()<<": "<<cwd<<" "<<username<< "$ ";
    
    getline(cin, inputline);
    
    past_cmds.push_back(trim(inputline));

    return trim(inputline);
}

char** Shell::vec_to_char_array(vector<string>& strings){
    char** result = new char * [strings.size() +1];
    for (int i = 0; i < strings.size(); i++ ){
        result[i] = (char*) strings[i].c_str();
    }
    result[strings.size()] = NULL;
    return result;
}

vector<string> Shell::split(string line, string _seperator){
    vector<string> strings;
    string buffy = "";
    string temp = "";
    bool count = false;
    bool dquoted = false;
    bool squoted = false;

    for (char& c: line){
        
        temp = c;

        if (c == '"' && !squoted) {
            dquoted = !dquoted;
            if (_seperator != " "){
                count = false;
                buffy += temp;
            }
        }
        else if (c == '\'' && !dquoted){
            squoted = !squoted;
            if (_seperator != " "){
                count = false;
                buffy += temp;
            }
        }
        else if (temp == _seperator && !count && !dquoted && !squoted){
            strings.push_back(buffy);
            buffy = "";
            count = true;
        }
        else if (temp != _seperator || dquoted || squoted){
            count = false;
            buffy += temp;
        }
        
    }
    strings.push_back(buffy);
    return strings;
}

string Shell::trim(const string& str){
    size_t first = str.find_first_not_of(' ');
    if (string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

void Shell::start_execution(){
    int in_def = dup(0);
    int out_def = dup(1);
    while (true) {
        dup2(in_def, 0);
        dup2(out_def, 1);

        kill_idle_children(); 

        string inputline = get_cmd_prompt();

        if (inputline == exit_cmd){break;}

        bg = inputline[inputline.size()-1] == '&';
        inputline = bg ? trim(inputline.substr(0,inputline.size()-1)) : inputline;

        execute_cmd(inputline,bg);
    }
    std::cout<<"Exiting..."<<std::endl;
}
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

const std::string now() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return buf;
}

void Shell::execute_cmd(const string& inputline, bool bg){
    int pid = fork();

    if (pid == 0) {
        vector<string> parts = split(inputline);
        char** args = vec_to_char_array(parts);

        execvp(args[0],args);
    }
    else {
        if (!bg){
            waitpid(pid,0,0);
        }
        else {
            bgs.push_back(pid);
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
    std::cout<<now()<<":"<<user<< "$ ";
    string inputline;
    getline(cin, inputline);
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
    for (char& c: line){
        temp = c;
        if (temp == _seperator && !count){
            strings.push_back(buffy);
            buffy = "";
            count = true;
        }
        else if (temp != _seperator){
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
    bool bg;
    while (true) {
        // check for background process that are done and kill them
        kill_idle_children(); 

        // get string inputs
        string inputline = get_cmd_prompt();

        // if input is exit command we exit the loop
        if (inputline == exit_cmd){break;}

        // if ampersand -> background procces, and remove ampersand
        bg = inputline[inputline.size()-1] == '&';
        inputline = bg ? trim(inputline.substr(0,inputline.size()-1)) : inputline;
        // fork
        // parse command and if is child execute command, else wait for child or set as background process
        execute_cmd(inputline,bg);
    }
    std::cout<<"Exiting..."<<std::endl;
}
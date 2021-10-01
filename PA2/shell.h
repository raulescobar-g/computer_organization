#ifndef _shell_h_                              /* include file only once */
#define _shell_h_
#include <string>
using namespace std;

class Shell {
    private:
        vector<int> bgs;
        string user;
        string exit_cmd;

    public:
        Shell(string _user, string _exit_cmd="exit"): user(_user), exit_cmd(_exit_cmd), bgs(){};
        void execute_cmd(const string& inputline, bool bg);
        void kill_idle_children();
        string get_cmd_prompt();
        char** vec_to_char_array(vector<string>& strings);
        vector<string> split(string line, string seperator=" ");
        string trim(const string& str);
        void start_execution();
};


#endif
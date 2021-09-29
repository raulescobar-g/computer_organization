#include <iostream>

using namespace std;

int main(){
    for (int i=0; i<4; i++){
    int cid = fork ();
    if (i < 2)
        wait (0);
    cout << "ID=" << getpid () << endl;
}

}
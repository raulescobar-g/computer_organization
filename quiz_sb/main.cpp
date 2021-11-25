#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
using namespace std;

struct data{
	int a;
	int b;
	data(int _a, int _b): a(_a), b(_b){}
	data(){}	
};

int main() {
	vector<char> buf(sizeof(data));
	data packet(10,11);
	memcpy(buf.data(), &packet, sizeof(data));

	data res;
	copy(buf.begin(), buf.end(), reinterpret_cast<char*>(&res));
	

	cout<<res.a<<endl;

  	return 0;
}
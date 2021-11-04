#include "common.h"
#include "FIFOreqchannel.h"
#include "BoundedBuffer.h"
#include "HistogramCollection.h"
#include <sys/wait.h>
#include <thread>
using namespace std;

struct packet {
	int p;
	DataRequest data;
	packet(const DataRequest& _data, int _p): data(_data), p(_p) {};
};

void patient_thread_function(int p, int n,BoundedBuffer& b){
    
	for (int i = 0 ; i < n; ++i){
		DataRequest d1(p,n*0.004,1);
		DataRequest d2(p,n*0.004,2);

		packet pack1(d1, p);
		packet pack2(d2, p);

		vector<char> buf1(sizeof(DataRequest));
		vector<char> buf2(sizeof(DataRequest));

		memcpy(buf1.data(), &pack1, sizeof(DataRequest));
		memcpy(buf2.data(), &pack2, sizeof(DataRequest));

		b.push(buf1);
		b.push(buf2);
	}
}


void worker_thread_function(BoundedBuffer& reqbuf, BoundedBuffer& resbuf, FIFORequestChannel& chan){
    double reply;
	int p;
	vector<char> buf(sizeof(double));
	while (1) {
		vector<char> req = reqbuf.pop();
		for (int i =0; i < 4; ++i){ p |= static_cast<int>(req[i+req.size() - sizeof(int)]) << (i * 8); }
		req.resize(req.size() - sizeof(int));
		chan.cwrite(req.data(), sizeof(DataRequest));
		chan.cread(&reply, sizeof(double));
		memcpy(buf.data(), &reply, sizeof(double));

		resbuf.push(buf);
		if (false) break;
	}
}
void histogram_thread_function (){
    /*
		Functionality of the histogram threads	
    */
}

void file_thread_function() {

}

int main(int argc, char *argv[]){

	int opt, 			// argument buffer
		p = 10,			// number of patient data to fetch (cumulative)
		h = 5,			// number of histogram threads
		b = 1024,		// bounded buffer size? still not sure come back to UPDATE
		w = 500,		// number of worker threads to create
		n = 15000,  	// number of data points per patient to fetch	
		m = 256; 		// default buffer memory for FIFO
	string filename = "";

	// size of bounded buffer, note: this is different from another variable buffercapacity/m
	// take all the arguments first because some of these may go to the server

	while ((opt = getopt(argc, argv, "f:p:h:w:b:n:m:")) != -1) {
		switch (opt) {
			case 'f':
				filename = optarg;
				break;
			case 'p':
				p = atoi(optarg);
				break;
			case 'h':
				h = atoi(optarg);
				break;
			case 'w':
				w = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 'n':
				n = atoi(optarg);
				break;
			case 'm':
				m = atoi(optarg);
				break;
		}
	}

	int pid = fork ();
	if (pid < 0){
		EXITONERROR ("Could not create a child process for running the server");
	}
	if (!pid){ // The server runs in the child process
		char int_str[50];
		snprintf(int_str, sizeof(int_str),"%d" ,m);

		char* args[] = {"./server" ,"-m" ,int_str ,nullptr};
		if (execvp(args[0], args) < 0){
			EXITONERROR ("Could not launch the server");
		}
	}

	FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
	BoundedBuffer request_buffer(b);
	BoundedBuffer response_buffers(b);
	HistogramCollection hc; 

	for (int i = 0 ; i < p; ++i){
		// Histogram histogrammy();
		// hc.add(&histogrammy);
		
	}

	thread patient_threads[p];
	thread worker_threads[w];
	thread histogram_threads[h];
	thread file_thread;

	FIFORequestChannel * chans[w];
	for (int i = 0; i < w; ++i){ 
		Request n_chan_req (NEWCHAN_REQ_TYPE);
		chan.cwrite (&n_chan_req, sizeof (n_chan_req));
		char chan_name[m];
		chan.cread(&chan_name,sizeof(chan_name));
		chans[i] = new FIFORequestChannel(chan_name, FIFORequestChannel::CLIENT_SIDE);
	}

	// STARTING THE TIMER NOW
	struct timeval start, end;
    gettimeofday (&start, 0);

    /* Starting all threads here */
	if (filename == "") for (int i = 0; i < p; ++i){ patient_threads[i] = thread(patient_thread_function,i+1,n,request_buffer); } 
	else file_thread = thread(file_thread_function);
	for (int i = 0; i < w; ++i){ worker_threads[i] = thread(worker_thread_function,request_buffer,response_buffers); }
	for (int i = 0; i < h ; ++i){ histogram_threads[i] = thread(histogram_thread_function); }
	


	/* Joining all threads here*/
	if (filename == "") for (int i = 0; i < p; ++i){ if (i < p) patient_threads[i].join();} else file_thread.join();
	for (int i = 0 ; i < w ; ++i){worker_threads[i].join();}
	for (int i = 0; i < h ; ++i){histogram_threads[i].join();}
	
	// ENDING TIMER NOW	
    gettimeofday (&end, 0);

	// deleting threads on heap
	delete[] patient_threads, worker_threads, histogram_threads;

    // print the results and time difference
	hc.print ();
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;
	
	// closing all the channels and deleting heap allocated chans
	Request q (QUIT_REQ_TYPE);
	for (int i = 0 ; i < p; ++i){
		chans[i]->cwrite(&q, sizeof (Request));
	}   
	delete[] chans;
    chan.cwrite (&q, sizeof (Request));
	// client waiting for the server process, which is the child, to terminate
	wait(0);
	cout << "Client process exited" << endl;

}

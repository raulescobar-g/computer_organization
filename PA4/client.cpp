#include "common.h"
#include "FIFOreqchannel.h"
#include "BoundedBuffer.h"
#include "HistogramCollection.h"
#include <sys/wait.h>
#include <thread>
#include <mutex>
using namespace std;



struct packet {
	int p;
	double reply;
	packet(int _p, double _reply): p(_p), reply(_reply) {}
};

void patient_thread_function(int p, int n,BoundedBuffer& b){
    
	for (int i = 0 ; i < n; ++i){
		
		DataRequest d1(p,n*0.004,1);
		DataRequest d2(p,n*0.004,2);

		vector<char> buf1(sizeof(DataRequest));
		vector<char> buf2(sizeof(DataRequest));

		memcpy(buf1.data(), &d1, sizeof(DataRequest));
		memcpy(buf2.data(), &d2, sizeof(DataRequest));
		
		b.push(buf1);
		b.push(buf2);
		
	}
}


void worker_thread_function(BoundedBuffer& reqbuf, BoundedBuffer& resbuf, FIFORequestChannel& chan, bool filereq){

	if (!filereq){
		while (true) {
			double reply;
			vector<char> buf(sizeof(packet));
			DataRequest requested(0,0.0,0);

			vector<char> req = reqbuf.pop();
			copy(req.begin(), req.end(), reinterpret_cast<char*>(&requested));
			
			
			chan.cwrite(&requested, sizeof(DataRequest));
			chan.cread(&reply, sizeof(double));
			packet response_packet(requested.person, reply);

			memcpy(buf.data(), &response_packet, sizeof(packet));
			resbuf.push(buf);
		}
	}
	else {
		;
	}
}
void histogram_thread_function(BoundedBuffer& resbuf, HistogramCollection& hc){
    
	while (1) {
		packet response(0,0.0);
		vector<char> req = resbuf.pop();
		copy(req.begin(), req.end(), reinterpret_cast<char*>(&response));
		hc.update(response.p, response.reply);
	}
}

void file_thread_function(string filename, int filelen, BoundedBuffer& reqbuf, int m, int b) {

	// for (int i = 0; i < (filelen/m); ++i){
	// 	FileRequest fm2(i*m, m);
	// 	int poopy_len = sizeof(FileRequest);
	// 	vector<char> poopy(poopy_len);
	// 	FileRequest packet(fm2, i, filename);
	// 	memcpy(poopy.data(), &packet, sizeof(filepacket));
	// 	reqbuf.push(poopy);
	// }
	// int i = (filelen/m);
	// int new_m = filelen - i*m;
	// FileRequest fm2(i*m, new_m);
	// int poopy_len = sizeof(filepacket);
	// vector<char> poopy(poopy_len);
	// filepacket packet(fm2, i, filename);
	// memcpy(poopy.data(), &packet, sizeof(filepacket));
	// reqbuf.push(poopy);
}

int main(int argc, char *argv[]){

	int opt, 			// argument buffer
		p = 15,			// number of patient data to fetch (cumulative)
		h = 5,			// number of histogram threads
		b = 1024,		// bounded buffer size? still not sure come back to UPDATE
		w = 500,		// number of worker threads to create
		n = 5000,  		// number of data points per patient to fetch	
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
		Histogram histogrammy(10,-2.0,2.0);
		hc.add(&histogrammy);
	}

	thread patient_threads[p];
	thread worker_threads[w];
	thread histogram_threads[h];
	//thread file_thread;

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
	if (filename == "") for (int i = 0; i < p; ++i){ patient_threads[i] = thread(patient_thread_function,i+1,n,ref(request_buffer)); } 
	//else {
		//file_thread = thread(file_thread_function,filename,filelen,request_buffer,m,b);
	//}
	for (int i = 0; i < w; ++i){ worker_threads[i] = thread(worker_thread_function,ref(request_buffer),ref(response_buffers),ref((*chans)[i]),false); }
	for (int i = 0; i < h ; ++i){ histogram_threads[i] = thread(histogram_thread_function,ref(response_buffers),ref(hc)); }
	


	/* Joining all threads here*/
	if (filename == "") {
		for (int i = 0; i < p; ++i){ 
			cout<<"here"<<endl;
			patient_threads[i].join();
			
		} 
	}//else file_thread.join();
	
	Request q (QUIT_REQ_TYPE);
	for (int i = 0 ; i < w; ++i){
		chans[i]->cwrite(&q, sizeof (Request));
		delete chans[i];
	}  

	for (int i = 0 ; i < w ; ++i){worker_threads[i].join();}
	for (int i = 0; i < h ; ++i){histogram_threads[i].join();}
	
	// ENDING TIMER NOW	
    gettimeofday (&end, 0);


    // print the results and time difference
	hc.print ();
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;
	
	// closing all the channels and deleting heap allocated chans
	 
	
    chan.cwrite (&q, sizeof (Request));
	// client waiting for the server process, which is the child, to terminate
	wait(0);
	cout << "Client process exited" << endl;

}

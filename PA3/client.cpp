#include "common.h"
#include "FIFOreqchannel.h"
#include <sys/wait.h>
#include <sys/time.h>
#include <vector>
#include <string>
using namespace std;

double get_data(const int p,const double t,const int e, FIFORequestChannel& chan2){
	DataRequest d (p, t, e);
	chan2.cwrite (&d, sizeof (DataRequest)); // question
	double reply;
	chan2.cread (&reply, sizeof(double)); //answer
	return reply;
}

int64 filelength(const string& filename,FIFORequestChannel& chan){
	FileRequest fm (0,0);
	int len = sizeof (FileRequest) + filename.size()+1;
	char buf2 [len];
	memcpy (buf2, &fm, sizeof (FileRequest));
	strcpy (buf2 + sizeof (FileRequest), filename.c_str());
	chan.cwrite (buf2, len);  
	int64 filelen;
	chan.cread (&filelen, sizeof(int64));
	return filelen;
}

void get_file(const int m, const int64 filelen, const string& filename, const int newfile, FIFORequestChannel& chan){

	for (int i = 0; i < (filelen/m); ++i){
		FileRequest fm2(i*m, m);
		int poopy_len = sizeof(FileRequest) + filename.size()+1;
		char poopy[poopy_len];
		memcpy(poopy, &fm2, sizeof(FileRequest));
		strcpy(poopy + sizeof(FileRequest), filename.c_str());
		chan.cwrite(poopy, poopy_len);
		char read[m];
		chan.cread(&read, m);
		int writer;
		if ((writer = write(newfile,read,sizeof(read))) < 0){
			perror("error writing to new file");
			exit(1);
		}
	}
	int i = (filelen/m);
	int new_m = filelen - i*m;
	FileRequest fm2(i*m, new_m);
	int poopy_len = sizeof(FileRequest) + filename.size()+1;
	char poopy[poopy_len];
	memcpy(poopy, &fm2, sizeof(FileRequest));
	strcpy(poopy + sizeof(FileRequest), filename.c_str());
	chan.cwrite(poopy, poopy_len);
	char read[new_m];
	chan.cread(&read, new_m);
	int writer;
	if ((writer = write(newfile,read,sizeof(read))) < 0){
		perror("error writing to new file");
		exit(1);
	}
		
}

// FIFORequestChannel req_channel(FIFORequestChannel& chan,vector<FIFORequestChannel> channels,const int m){
	//could implement to have more abstraction and generality
// }

string rounder(double& a){
	string num = to_string(a);
	num.erase ( num.find_last_not_of('0') + 1, string::npos );
	if (num[num.size()-1] == '.' ) {num.erase(num.size()-1);}
	return num;
}

void k_requests(const int p, FIFORequestChannel& chan){
	double t = 0.0, reply1, reply2;
	string newfile = "received/x1.csv";
	int fd;
	string reply;

	if ((fd = open(newfile.c_str(),O_WRONLY|O_CREAT)) < 0) {
		perror("creating file to write 1k entries");
		exit(1);
	}

	for (int i = 0; i < 1000; ++i){
		reply1 = get_data(p,t,1,chan);
		reply2 = get_data(p,t,2,chan);
		reply = rounder(t)+","+ rounder(reply1)+","+rounder(reply2)+"\n";
		t+= 0.004;

		int writer;
		if ((writer = write(fd,reply.c_str(),reply.size())) < 0){
			perror("error writing to new file");
			exit(1);
		}
	}
}

void handle_request(FIFORequestChannel& chan1, const int p, int e, const int m, const double t, const string& filename){
	if (filename == "" && t >= 0.0) {
		double reply;
		if (e == -1){
			e = 1;
			reply = get_data(p,t,e,chan1); // abstracted out the data query
			isValidResponse(&reply) ? cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply << endl: cout<<"Not valid response"<<endl;
			e = 2;
			reply = get_data(p,t,e,chan1); // abstracted out the data query
			isValidResponse(&reply) ? cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply << endl: cout<<"Not valid response"<<endl;
		}
		else {
			reply = get_data(p,t,e,chan1); // abstracted out the data query
			isValidResponse(&reply) ? cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply << endl: cout<<"Not valid response"<<endl;
		}
	}
	else if (filename == "" && t < 0) {
		// here we go into a loop making 1k requests and putting in x1.csv file
		//cout<<"Getting 1k requests...........";
		k_requests(p,chan1); 
		//cout<<"Done"<<endl;
	}
	else { // god plz 
		//cout<<"Getting file request..........";
		int64 filelen = filelength(filename, chan1); // abstracted out the filelength query
		// if (isValidResponse(&filelen)){
		// 	cout << "File length is: " << filelen << " bytes" << endl;
		// }

		int newfile;
		string real_filename = "received/"+filename;
		
		if ((newfile = open(real_filename.c_str(),O_WRONLY|O_CREAT)) < 0){
			perror("openning file to write to.");
			exit(1);
		}
		
		get_file(m,filelen,filename,newfile,chan1); // abstracted away getting the file by chunks
		//cout<<"Done"<<endl;
	}
}

// void close_channels(vector<FIFORequestChannel>& channels, FIFORequestChannel& chan3){
	// could implement to have more abstraction and generality
// }

void server_up(const int m){
	//cout<<"Starting server...............";
	int pid = fork ();
	if (pid < 0){
		EXITONERROR ("Could not create a child process for running the server");
	}
	if (!pid){ // The server runs in the child process
		char int_str[50];
		snprintf(int_str, sizeof(int_str),"%d" ,m);
		char* args[] = {"./server", "-m", int_str ,nullptr};
		if (execvp(args[0], args) < 0){
			EXITONERROR ("Could not launch the server");
		}
	}
	//cout<<"Done"<<endl;
}

int main(int argc, char *argv[]){
	// init to default parameters
	vector<FIFORequestChannel> channels;
	struct timeval start, end;
	int p = -1, e = -1, m = 256, opt;
	double t = -1.0;
	string filename = "";
	bool new_channel = false;

	// get parameters 
	while ((opt = getopt(argc, argv, "f:p:t:e:m:c")) != -1) {
		switch (opt) {
			case 'f':
				filename = optarg;
				break;
			case 't':
				t = atof(optarg);
				break;
			case 'p':
				p = atoi(optarg);
				break;
			case 'e':
				e = atoi(optarg);
				break;
			case 'm':
				m = atoi(optarg);
				break;
			case 'c':
				new_channel = true;
				break;
		}
	}

	// child process initializes the server we will be communicating with
	server_up(m);

	// mandatory initialize control channel
	FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);

	// if new channel requested then we handle request in the new channel otherwise we handle requests with control channel
	if (new_channel){
		Request n_chan_req (NEWCHAN_REQ_TYPE);
		chan.cwrite (&n_chan_req, sizeof (n_chan_req));
		char chan_name[m];
		chan.cread(&chan_name,sizeof(chan_name));
		FIFORequestChannel n_chan(chan_name, FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(n_chan);
		handle_request(n_chan,p,e,m,t,filename);

		Request q (QUIT_REQ_TYPE);
		n_chan.cwrite(&q, sizeof(q));
	}
	else {
		gettimeofday(&start, NULL);
		handle_request(chan,p,e,m,t,filename);
		gettimeofday(&end,NULL);
		int microseconds = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
		cout<<"Time taken: "<<microseconds<<endl;
	}

	// loop through children channels and close them, then close the control channel and wait for server chil process
	//cout<<"Children channels closing.....";
	Request q (QUIT_REQ_TYPE);
	chan.cwrite (&q, sizeof (q));
	//cout<<"Done"<<endl;
	
	// client waiting for the server process, which is the child, to terminate
	
	wait(0);
	//cout<<"Done" << endl;
	//cout<<"Client process exiting........";
	//cout<<"Done"<< endl;
}

// when trying to quit using vector, it says it cant read the request to quit from child channel
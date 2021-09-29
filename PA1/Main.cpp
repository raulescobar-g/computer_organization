#include "Ackerman.h"
#include "BuddyAllocator.h"
#include <unistd.h>
#include <iostream>
using namespace std;

void easytest(BuddyAllocator* ba){
	// be creative here
	// know what to expect after every allocation/deallocation cycle

	// here are a few examples
	
	ba->printlist();
	// allocating a byte
	
	char * mem = ba->alloc (1);
	ba->printlist();

	ba->free (mem); // give back the memory you just allocated
	ba->printlist(); // shouldn't the list now look like as in the beginning
} 

int main(int argc, char ** argv) {

	int c, 
		basic_block_size = 128,
		memory_length = 128 * 1024;

  	while (((c = getopt(argc,argv,"b:s:")) != -1)) {
		switch (c){
			case 'b':
				basic_block_size = atoi(optarg);
				break;
			case 's':
				memory_length = atoi(optarg);
				break;
			case '?':
				std::cout<<"Only -b or -s flag are valid"<<std::endl;
				return 1;
			default:
				std::cout<<"How did you even get to this error bro?"<<std::endl;
		}
  	}

	// create memory manager
	BuddyAllocator * allocator = new BuddyAllocator(basic_block_size, memory_length);

	// the following won't print anything until you start using FreeList and replace the "new" with your own implementation
	easytest(allocator);

	
	// stress-test the memory manager, do this only after you are done with small test cases
	Ackerman* am = new Ackerman ();
	am->test(allocator); // this is the full-fledged test. 
	
	// destroy memory manager
	delete allocator;
}

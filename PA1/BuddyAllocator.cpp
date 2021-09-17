#include "BuddyAllocator.h"
#include <iostream>
#include <math.h>
using namespace std;

BlockHeader* BuddyAllocator::getbuddy (BlockHeader * addr){
	return (BlockHeader*) ((char*)addr + addr->block_size); 
}

bool BuddyAllocator::arebuddies (BlockHeader* block1, BlockHeader* block2){
	if (block1 == getbuddy(block2)){
		return true;
	}
	else if (block2 == getbuddy(block1)){
		return true;
	} 
	else{
		return false;
	}
}

BlockHeader* BuddyAllocator::merge(BlockHeader* block1, BlockHeader* block2){
	int index = (int) log2(ceil( (double) (block1->block_size) / basic_block_size)); // this might be wrong lmao
	
	FreeList[index].remove(block1);
	FreeList[index].remove(block2);

	block1->block_size *= 2;
	FreeList[index+1].insert(block1);

	return block1;

}

BlockHeader* BuddyAllocator::split(BlockHeader* b){
	int bs = b->block_size;
	b->block_size /= 2;
	b->next = nullptr;

	BlockHeader* sh = getbuddy(b);
	sh->next = nullptr;
	sh->block_size = b->block_size;
	//BlockHeader* temp = new (sh) BlockHeader (b->block_size);
	return sh;
}

BuddyAllocator::BuddyAllocator (int _basic_block_size, int _total_memory_length){
	total_memory_size = _total_memory_length; 
	basic_block_size = _basic_block_size;

	start = new char[total_memory_size];

	int l = log2(total_memory_size / basic_block_size);

	for (int i = 0; i < l; i++){
		FreeList.push_back(LinkedList());
	}
	FreeList.push_back(LinkedList((BlockHeader*) start)); // last element in LL
	BlockHeader* h = new (start) BlockHeader(total_memory_size);
}

BuddyAllocator::~BuddyAllocator (){
	delete [] start;
}

char* BuddyAllocator::alloc(int _length) { 
  	int x = _length + sizeof(BlockHeader);
	int index = (int) log2(ceil( (double) x / basic_block_size));

	int blockSizeReturn = (1 << index) * basic_block_size;

	if (FreeList[index].head != nullptr){
		BlockHeader* b = FreeList[index].remove();
		return (char*) (b+1);
	}
	else {
		int const_index = index;
		for (;index < FreeList.size() ; index++){
			if (FreeList[index].head != nullptr){
				break;
			}
		}
		if (index >= FreeList.size()){
			return nullptr;
		}

		while (index > const_index){
			BlockHeader* b = FreeList[index].remove();
			BlockHeader* shb = split(b);
			--index;
			FreeList[index].insert(b);
			FreeList[index].insert(shb);
		}
		return (char*) (FreeList[index].remove()+1);
	}
}

int BuddyAllocator::free(char* _a) {
  //shift _a from the writable memory to the blockheader with metadata
	//check if block to be deleted is in FreeList
	//add block to free list
	//if buddy is free merge recursively 

  return 0;
}

void BuddyAllocator::printlist (){
  cout << "Printing the Freelist in the format \"[index] (block size) : # of blocks\"" << endl;
  for (int i=0; i<FreeList.size(); i++){
    cout << "[" << i <<"] (" << ((1<<i) * basic_block_size) << ") : ";  // block size at index should always be 2^i * bbs
    int count = 0;
    BlockHeader* b = FreeList [i].head;
    // go through the list from head to tail and count
    while (b){
      count ++;
      // block size at index should always be 2^i * bbs
      // checking to make sure that the block is not out of place
      if (b->block_size != (1<<i) * basic_block_size){
        cerr << "ERROR:: Block is in a wrong list" << endl;
        exit (-1);
      }
      b = b->next;
    }
    cout << count << endl;  
  }
}


#include "BuddyAllocator.h"
#include <iostream>
#include <math.h>
using namespace std;

BlockHeader* BuddyAllocator::getbuddy (BlockHeader * addr){
	int offset = (int)((char *) addr - start);
	int buddy_offset = offset ^ addr->block_size;
	return (BlockHeader*) (start + buddy_offset);
}

bool BuddyAllocator::arebuddies (BlockHeader* block1, BlockHeader* block2){
	return (getbuddy(block1) == block2);
}

BlockHeader* BuddyAllocator::merge(BlockHeader* block1, BlockHeader* block2){
	block1->block_size *= 2;
	return block1;
}

BlockHeader* BuddyAllocator::split(BlockHeader* b){
	int bs = b->block_size;
	b->block_size /= 2;
	b->next = nullptr;

	BlockHeader* sh = getbuddy(b);
	BlockHeader* temp = new (sh) BlockHeader (b->block_size);
	return temp; //could be sh not temp
}

BuddyAllocator::BuddyAllocator(int _basic_block_size, int _total_memory_length){
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

BuddyAllocator::~BuddyAllocator(){
	delete [] start;
}

char* BuddyAllocator::alloc(int _length) { 
  	int x = _length + sizeof(BlockHeader);
	int index = (int) log2(ceil( (double) x / basic_block_size));

	int blockSizeReturn = (1 << index) * basic_block_size;

	if (FreeList[index].head != nullptr){
		BlockHeader* b = FreeList[index].remove();
		b->isFree = 0;
		return (char*) (b+1);
	}
	else {

		const int const_index = index;

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
			FreeList[index].insert(shb);
			FreeList[index].insert(b);
		}
		BlockHeader* block = FreeList[index].remove();
		block->isFree = 0;
		return (char*) (block+1);
	}
}

int BuddyAllocator::free(char* _a) {
	BlockHeader * b = (BlockHeader*) (_a - sizeof(BlockHeader));
	
	while (true){
		int size = b->block_size;
		b->isFree = 1;
		int index = (int) log2(ceil( (double) b->block_size / basic_block_size));

		if (index == FreeList.size()-1) {
			FreeList[index].insert(b);
			break;
		}

		BlockHeader* buddy = getbuddy(b);

		if (buddy->isFree) {
			FreeList[index].remove(buddy);
			if (b > buddy){
				std::swap(b,buddy);
			}
			b = merge(b,buddy);
		}
		else{
			FreeList[index].insert(b); //index could be index+1 idk
			break;
		}
	}

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


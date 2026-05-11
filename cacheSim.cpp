#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using std::vector;

//addr consists out of tag bits, then set bits and then offset bits (LSB)
class Address 
{
	public:

		unsigned int init_addr;
		unsigned int addr_tag;
		unsigned int addr_set;
		unsigned int addr_offset;

		Address(unsigned int given_addr,  int sets_num, int block_size) : 
			init_addr(given_addr), addr_tag((given_addr / block_size) / sets_num),
			addr_set((given_addr/block_size) % sets_num), addr_offset(given_addr % block_size)
		{}

};

// way represents a cache memory block that is a part of a set
class Way 
{
	public:

		bool is_valid = false;
		bool is_dirty = false;
		unsigned int way_tag = 0;
		
		// a counter for LRU use. Works like time stamp, if zero - we used it now. Higher counter means least used.
		int last_used = 0;
		
		Way() = default; 
		Way(bool valid ,bool dirty, unsigned int new_tag) : 
			is_valid(valid), is_dirty(dirty), way_tag(new_tag)
		{}

};

//the cache consists of sets, each set contains vec of ways like we learned in the lecture.
struct MemorySet
{
	vector <Way> ways;

	//the num of the ways in each set depends on it's associativity
	MemorySet(int associativity) :
		ways(associativity)
	{}

	void lastUsedUpdate(unsigned int just_visited){
		for (unsigned int i = 0; i < ways.size(); i++){
			if(i != just_visited){
				ways[i].last_used++;	     												//add 1 to the time stamp of the rest
			}
		}
		ways[just_visited].last_used = 0;    												//reset the time stamp of the last visited block
	}

	int getLeastRecentlyUsedBlock(){	
		int LRU_block_index = 0;	
		
		//we want to evict the LRU block, that means the block with the highest time stamp == highest 'last_used'.
		for (unsigned int i = 1; i < ways.size(); i++){
			if(ways[LRU_block_index].last_used < ways[i].last_used){
				LRU_block_index = i;
			}
		}
		return LRU_block_index;
	
	}

};

//Binds all the classes above, consists of sets that contain ways (memory blocks).
class Cache {
	private:
		
		int block_size;																		//2^given_block_size
		int cache_size;																		//2^given_cache_size
		int cache_associativity;															//2^given_cache_associativity
		int cache_access_time;																//in cycles

		vector <MemorySet> sets;
	
	public:
		Cache *next_Cache = nullptr;
		Cache *prev_Cache = nullptr;
		int sets_num;
		
		Cache(int block_size_exp, int cache_size_exp, int cache_associativity_exp, int cache_access_time_exp) :
			block_size(pow(2, block_size_exp)), cache_size(pow(2, cache_size_exp)), 
			cache_associativity(pow(2, cache_associativity_exp)), cache_access_time(cache_access_time_exp),
			sets_num(0)
		{
			sets_num = cache_size / (block_size*cache_associativity);
			sets.resize(sets_num, MemorySet(cache_associativity));
		}

		// tries to find address in cache, gets hit/miss
		int findInCache(unsigned int given_address, bool write_allocate, bool write_mode){
			Address address_to_find(given_address, sets_num, block_size);
			MemorySet &set_index = sets[address_to_find.addr_set];

			//going through the ways in each set and trying to find a hit.
			for (int i = 0; i < cache_associativity; i++){
				Way &way = set_index.ways[i];
				if(way.way_tag == address_to_find.addr_tag && way.is_valid == true){		//we have a hit!
					if(write_mode == true){
						way.is_dirty = true;												//if we in 'w' mode, it's dirty
					}
					set_index.lastUsedUpdate(i);											//update time stamp
					return i;
				}
			}

			//didn't find it == miss
			return -1;
		}

		int getBlockToFill (int set_index, unsigned int given_address, bool write_allocate, bool write_mode){
			for (int i = 0; i < cache_associativity; i++){									//trying to find free block
				Way &way = sets[set_index].ways[i];
				if(way.is_valid == false){													//not valid means not updated or just empty
					return i;
				}
			}
			
			int to_evict = sets[set_index].getLeastRecentlyUsedBlock();						//didn't find free way, need to evict
			Way &to_evict_way = sets[set_index].ways[to_evict];
			
			if (to_evict_way.is_dirty == true){												//if dirty in L1, need to be updated in L2.
				if(next_Cache != nullptr){
					int to_update_way = -1;
					for (int i = 0; i < next_Cache->cache_associativity; i++){				//find to_update in L2
						Way &way = next_Cache->sets[set_index].ways[i];
						if(way.way_tag == to_evict_way.way_tag && way.is_valid == true){
							to_update_way = i;
							break;
						}
					}
					if (to_update_way != -1){												//we found it in L2 so we need to update it
						next_Cache->sets[set_index].ways[to_update_way].is_dirty = true;
						next_Cache->sets[set_index].lastUsedUpdate(to_update_way);
					}
				}
			}
			if (next_Cache == nullptr){														//we're in L2 and need to snoop for to_evict in L1
				int to_evict_L1 = -1;
				for (int i = 0; i < prev_Cache->cache_associativity; i++){
					Way &way = prev_Cache->sets[set_index].ways[i];
					if(way.way_tag == to_evict_way.way_tag && way.is_valid == true){
						to_evict_L1 = i;
						break;
					}
				}
				if(to_evict_L1 != -1){														//found in L1 and need to be evicted
					prev_Cache->sets[set_index].ways[to_evict_L1].is_valid = false;
				}
			}
			sets[set_index].ways[to_evict].is_valid = false;
			return to_evict;
		}

		void addToCache(unsigned int given_address, bool write_allocate, bool write_mode){
			Address new_adress (given_address, sets_num, block_size);
			MemorySet &adress_set = sets[new_adress.addr_set];
			
			int way_index = getBlockToFill (new_adress.addr_set, given_address, write_allocate, write_mode);
			Way &to_fill_way = adress_set.ways[way_index];
			
			to_fill_way = Way(true,	write_mode, new_adress.addr_tag);						//update the block to have the new data
			adress_set.lastUsedUpdate(way_index);											//update the time stamp
		}
};


int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	//added from here
	Cache L1_cache(BSize, L1Size, L1Assoc, L1Cyc);
	Cache L2_cache(BSize, L2Size, L2Assoc, L2Cyc);
	L1_cache.next_Cache = &L2_cache;
	L2_cache.prev_Cache = &L1_cache;

	int L1_hit_num = 0;
	int L2_hit_num = 0;
	int L1_miss_num = 0;
	int L1_access_num = 0;

	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);
		
		bool write_mode = false;
		if (operation == 'w'){
			write_mode = true;
		}

		L1_access_num++;
		//hit in L1
		if (L1_cache.findInCache(num, WrAlloc, write_mode) != -1){
			L1_hit_num++;
		}
		//miss in L1, trying L2
		else{
			L1_miss_num++;
			if (L2_cache.findInCache(num, WrAlloc, write_mode) != -1){						//hit in L2
				L2_hit_num++;
			}
			else{																			//miss in L2
				if (write_mode == false || (write_mode == true && WrAlloc == true)){
				L2_cache.addToCache(num, WrAlloc, write_mode);								//if write allocate, we need to add it to L2 from mem
				}
			}
			if (write_mode == false || (write_mode == true && WrAlloc == true)){
				L1_cache.addToCache(num, WrAlloc, write_mode);								//the data in L2 and now we need to bring it to L1
			}
		}
	}

	double L1MissRate = 1-(static_cast<double>(L1_hit_num)/L1_access_num);
	double L2MissRate = 1-(static_cast<double>(L2_hit_num)/L1_miss_num);
	//from the lecture: t(avgAccTime) = (Hit time * Hit Rate) + (Miss time * Miss Rate)
	double avgAccTime = L1Cyc * (1 - L1MissRate) + ((L1Cyc + L2Cyc) * L1MissRate * (1 - L2MissRate)) + ((L1Cyc + L2Cyc + MemCyc) * L1MissRate * L2MissRate);

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}

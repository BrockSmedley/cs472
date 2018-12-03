/*
	Cache Simulator Implementation by Justin Goins
	Oregon State University
	Fall Term 2018
*/

#include "CacheController.h"
#include <iostream>
#include <fstream>
#include <regex>
#include <cmath>
#include <sstream>
#include <string>
#include <cstdlib>

using namespace std;

CacheController::CacheController(ConfigInfo ci, char* tracefile) {
	// store the configuration info
	this->ci = ci;
	this->inputFile = string(tracefile);
	this->outputFile = this->inputFile + ".out";
	// compute the other cache parameters
	this->numByteOffsetBits = log2(ci.blockSize);
	this->numSetIndexBits = log2(ci.numberSets);

	// initialize the counters
	this->globalCycles = 0;
	this->globalHits = 0;
	this->globalMisses = 0;
	this->globalEvictions = 0;
	
	int numBlocks = this->ci.numberSets * this->ci.associativity;
	this->sentinel = 999999999;
	
	cout << "# BLOCKS: " << numBlocks << endl;

	//  init cache by placing a -1 at each cache location
	static vector<unsigned long int> cache;
	this->cache = &cache;
	for (int i = 0; i < numBlocks; i++){
		cache.push_back(this->sentinel);
	}

	// init "LRU offset registers" w/ 0s
	static vector<unsigned int> lruOffset;
	this->setlruOffset = &lruOffset;
	for (int i = 0; i < this->ci.numberSets; i++){
		lruOffset.push_back(0);
	}

	// manual test code to see if the cache is behaving properly
	// will need to be changed slightly to match the function prototype
	/*
	cacheAccess(false, 0);
	cacheAccess(false, 128);
	cacheAccess(false, 256);

	cacheAccess(false, 0);
	cacheAccess(false, 128);
	cacheAccess(false, 256);
	*/
}

/*
	Starts reading the tracefile and processing memory operations.
*/
void CacheController::runTracefile() {
	cout << "Input tracefile: " << inputFile << endl;
	cout << "Output file name: " << outputFile << endl;
	
	// process each input line
	string line;
	// define regular expressions that are used to locate commands
	regex commentPattern("==.*");
	regex instructionPattern("I .*");
	regex loadPattern(" (L )(.*)(,[[:digit:]]+)$");
	regex storePattern(" (S )(.*)(,[[:digit:]]+)$");
	regex modifyPattern(" (M )(.*)(,[[:digit:]]+)$");

	// open the output file
	ofstream outfile(outputFile);
	// open the output file
	ifstream infile(inputFile);
	// parse each line of the file and look for commands
	while (getline(infile, line)) {
		// these strings will be used in the file output
		string opString, activityString;
		smatch match; // will eventually hold the hexadecimal address string
		unsigned long int address;
		// create a struct to track cache responses
		CacheResponse response;

		// ignore comments
		if (std::regex_match(line, commentPattern) || std::regex_match(line, instructionPattern)) {
			// skip over comments and CPU instructions
			continue;
		} else if (std::regex_match(line, match, loadPattern)) {
			cout << "Found a load op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
			cacheAccess(&response, false, address);
			outfile << " " << response.cycles << (response.hit ? " hit" : " miss") << (response.eviction ? " eviction" : "");
		} else if (std::regex_match(line, match, storePattern)) {
			cout << "Found a store op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
			cacheAccess(&response, true, address);
			outfile << " " << response.cycles << (response.hit ? " hit" : " miss") << (response.eviction ? " eviction" : "");
		} else if (std::regex_match(line, match, modifyPattern)) {
			cout << "Found a modify op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
			// first process the read operation
			cacheAccess(&response, false, address);
			string tmpString; // will be used during the file output
			tmpString.append(response.hit ? " hit" : " miss");
			tmpString.append(response.eviction ? " eviction" : "");
			unsigned long int totalCycles = response.cycles; // track the number of cycles used for both stages of the modify operation
			// now process the write operation
			cacheAccess(&response, true, address);
			tmpString.append(response.hit ? " hit" : " miss");
			tmpString.append(response.eviction ? " eviction" : "");
			totalCycles += response.cycles;
			outfile << " " << totalCycles << tmpString;
		} else {
			throw runtime_error("Encountered unknown line format in tracefile.");
		}
		outfile << endl;
	}
	// add the final cache statistics
	outfile << "Hits: " << globalHits << " Misses: " << globalMisses << " Evictions: " << globalEvictions << endl;
	outfile << "Cycles: " << globalCycles << endl;

	infile.close();
	outfile.close();
}

/*
	Calculate the block index and tag for a specified address.
*/
CacheController::AddressInfo CacheController::getAddressInfo(unsigned long int address) {
	AddressInfo ai;
	// this code should be changed to assign the proper index and tag
	ai.tag = address; // TODO: does this need to be changed??
	ai.setIndex = address % (unsigned int)this->ci.numberSets;
	return ai;
}

/*
	This function allows us to read or write to the cache.
	The read or write is indicated by isWrite.
*/
void CacheController::cacheAccess(CacheResponse* response, bool isWrite, unsigned long int address) {
	// determine the index and tag
	AddressInfo ai = getAddressInfo(address);
	vector<unsigned int> emptyBlocks;
	vector<unsigned int> occupiedBlocks;

	cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;
	
	unsigned int blockAddress = this->ci.associativity * ai.setIndex;
	cout << "cache blocks online: " << this->cache->size() << endl;

	// check each block in the set
	// pretend this happens all at once
	// Determine outcome: hit/miss/eviction
	unsigned int i;
	response->eviction = false;
	response->dirtyEviction = false;
	for (i = 0; i < this->ci.associativity; i++)
	{
		cout << "BLOCKADDR: " << blockAddress + i << endl;
		response->hit = false;

		if (this->cache->data()[blockAddress + i] == this->sentinel){ // not occupied; miss
			emptyBlocks.push_back(blockAddress + i);
		}
		else if (this->cache->data()[blockAddress + i] == ai.tag){ // hit
			cout << "HIT" << endl;
			response->hit = true;
			break;
		}
		else { // occupied; potential miss
			occupiedBlocks.push_back(blockAddress + i);
		}
	}

	if (!response->hit)
	{ // if miss, set new block address and write tag to its cache location
		// if we have empty blocks, choose the first one
		if (!emptyBlocks.empty()){
			blockAddress = emptyBlocks.front();
		}
		// otherwise, choose the first occupied block + LRU offset
		else if (!occupiedBlocks.empty()){
			// LRU runs by default. If random, just pick a new random offset.
			if (this->ci.rp == ReplacementPolicy::Random){
				srand(time(NULL));
				cout << rand() << endl;
				int r = rand() % this->ci.associativity;
				cout << "RANDOM BLOCK OFFSET: " << r << endl;
				blockAddress = occupiedBlocks.front() + (r);
			}
			else
				blockAddress = occupiedBlocks.front() + this->setlruOffset->data()[ai.setIndex];

			// use round-robin technique to increment LRU
			if (occupiedBlocks.size() == this->ci.associativity)
			{
				unsigned int offset = this->setlruOffset->data()[ai.setIndex];
				cout << "OFFSET: " << offset << endl;
				this->setlruOffset->data()[ai.setIndex] = (offset + 1) % this->ci.associativity;
				response->eviction = true;
				if (isWrite)
					response->dirtyEviction = true;
			}
		}

		cout << "cache[" << blockAddress << "] = " << cache->data()[blockAddress] << endl;
		this->cache->data()[blockAddress] = ai.tag;
		cout << "Stored " << ai.tag << " at cache[" << blockAddress << "]" << endl;
	}	

	// your code needs to update the global counters that track the number of hits, misses, and evictions
	updateCycles(response, isWrite);
	this->globalCycles += response->cycles;

	if (response->hit)
		cout << "Address " << std::hex << address << " was a hit." << endl;
	else
		cout << "Address " << std::hex << address << " was a miss." << endl;

	cout << "-----------------------------------------" << endl;

	return;
}

/*
	Compute the number of cycles used by a particular memory operation.
	This will depend on the cache write policy.
*/
void CacheController::updateCycles(CacheResponse* response, bool isWrite) {
	// your code should calculate the proper number of cycles
	int c = 0;

	if (isWrite){ /* on write */
		if (this->ci.wp == WritePolicy::WriteThrough)
		{
			// write to memory and cache
			c += this->ci.cacheAccessCycles + this->ci.memoryAccessCycles;
		}
		else
		{ // in write-back, if we hit on a write, then we don't need to access MM.
			c += this->ci.cacheAccessCycles;
		}

		if (response->hit){
			this->globalHits += 1;
		}
		else { // miss
			this->globalMisses += 1;
		}
	}
	else { /* on read */
		if (response->hit){
			c += this->ci.cacheAccessCycles;
			this->globalHits += 1;
		}
		else { // miss
			// check cache, fail, check memory, then update cache
			c += this->ci.cacheAccessCycles * 2 + this->ci.memoryAccessCycles;
			this->globalMisses += 1;
		}
	}

	if (response->eviction || response->dirtyEviction){
		// on eviction, we have to wait until we write to memory and then write to cache
		c += this->ci.cacheAccessCycles + this->ci.memoryAccessCycles;
		this->globalEvictions += 1;
	}

	response->cycles = c;
}
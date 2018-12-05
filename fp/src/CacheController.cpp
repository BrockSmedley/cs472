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
	static vector<std::queue<unsigned int>> lruOffset;
	this->setlruOffset = &lruOffset;
	for (unsigned int i = 0; i < this->ci.numberSets; i++){
		lruOffset.push_back(queue<unsigned int>());
		lruOffset[i].push(0);
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

// Function to extract k bits from p position
// and returns the extracted value as integer
unsigned long int bitExtracted(unsigned long int number, unsigned int k, unsigned int p)
{
	return (((1 << k) - 1) & (number >> (p - 1)));
	// thanks https://www.geeksforgeeks.org/extract-k-bits-given-position-number/
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
	// assign the proper index and tag
	unsigned int tagSize = 64 - this->numSetIndexBits - this->numByteOffsetBits;
	ai.tag = bitExtracted(address, tagSize, 0);
	//ai.tag = address;// TODO: CHANGE ME
	unsigned long int blockAddr = address / (long)this->ci.blockSize;
	cout << "BLOCK ADDRESS: " << blockAddr << endl;
	ai.setIndex = blockAddr % (unsigned long int)this->ci.numberSets;
	return ai;
}

void CacheController::updateLRU(){

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
	unsigned int offset = 0;

	cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;
	
	unsigned int blockNumber = this->ci.associativity * ai.setIndex;
	//cout << "cache blocks online: " << this->cache->size() << endl;
	//cout << "blockNum Init: " << blockNumber << endl;

	// check each block in the set
	// pretend this happens all at once
	// Determine outcome: hit/miss/eviction
	unsigned int i;
	response->eviction = false;
	response->dirtyEviction = false;
	for (i = 0; i < this->ci.associativity; i++)
	{
		cout << "BLOCK NUMBER: " << blockNumber + i << endl;
		response->hit = false;

		if (this->cache->data()[blockNumber + i] == this->sentinel){ // not occupied; miss
			emptyBlocks.push_back(i);
		}
		else if (this->cache->data()[blockNumber + i] == ai.tag){ // hit
			cout << "HIT" << endl;
			response->hit = true;
			break;
		}
		else { // occupied; potential miss
			occupiedBlocks.push_back(i);
		}
	}

	if (!response->hit)
	{ // if miss, set new block address and write tag to its cache location
		// if we have empty blocks, choose the first one
		if (!emptyBlocks.empty()){
			offset = emptyBlocks.front();
		}
		// otherwise, choose the first occupied block + LRU offset
		else if (!occupiedBlocks.empty()){
			// LRU runs by default. If random, just pick a new random offset.
			if (this->ci.rp == ReplacementPolicy::Random){
				srand(time(NULL));
				cout << rand() << endl;
				int r = rand() % this->ci.associativity;
				cout << "RANDOM BLOCK OFFSET: " << r << endl;
				offset = r;
			}
			else{
				offset = this->setlruOffset->data()[ai.setIndex].front();
				//this->setlruOffset->data()[ai.setIndex].pop();
			}

			// update evictions
			if (occupiedBlocks.size() == this->ci.associativity){
				cout << "OFFSET: " << offset << endl;
				response->eviction = true;
				if (isWrite)
					response->dirtyEviction = true;
			}
		}

		blockNumber = (blockNumber + offset);
		

		cout << "cache[" << blockNumber << "] = " << cache->data()[blockNumber] << endl;
		this->cache->data()[blockNumber] = ai.tag;
		cout << "Stored " << ai.tag << " at cache[" << blockNumber << "]" << endl;
	}

	// update LRU
	if (this->setlruOffset->data()[ai.setIndex].size() == this->ci.associativity)
		this->setlruOffset->data()[ai.setIndex].pop();
	if (this->setlruOffset->data()[ai.setIndex].back() != offset)
		this->setlruOffset->data()[ai.setIndex].push(offset);
	//cout << "dangol offset man: " << offset << endl;

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
			// check memory, then update cache
			c += this->ci.cacheAccessCycles + this->ci.memoryAccessCycles;
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
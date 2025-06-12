#ifndef CHUNK_H
#define CHUNK_H

#include <cstring>
#include <iostream>

#include <exception>

#ifdef _WIN32
#include <bitset>
#else
#include <bits/stdc++.h>
#endif


class Chunk {
private:
	static const unsigned int CHUNK_SIZE = 8; //size of chunk (n*n*n)

public:
	std::bitset<CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE> data; // data of chunk, if bit is 1 then block in that location else no block in that location

	Chunk();
	~Chunk();

	void toggleBlock(int x, int y, int z);

	inline unsigned int getChunkSize() const { return CHUNK_SIZE; };

	void printData() const;

	//static int coordsToDataLoc(int x, int y, int z);
};


#endif // !CHUNK_H

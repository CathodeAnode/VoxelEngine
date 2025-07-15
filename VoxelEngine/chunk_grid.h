#ifndef CHUNKGRID_H
#define CHUNKGRID_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glfw/glfw3.h>

#include <unordered_map>
#include <stdexcept>
#include <limits>
#include "iostream"

#include "chunk.h"
#include "types.h"

#define MAX_GRID_INT 1048575

template<typename ChunkType> class ChunkGrid {
public:

	ChunkGrid(char* serializedData); // load from serialized data

	// generate flat world with given size
	ChunkGrid(int worldSize) {
		chunks.reserve(11000);
	}

	~ChunkGrid() {
		chunks.clear();
	}

	ChunkGrid() {};

	//TODO: change to take pointers instead of making a copy of chunk (optmization)
	void addChunk(const ChunkType& chunk, const glm::ivec3& chunkGridLocation) {
		uint64_t chunkIndex = getChunkIndex(chunkGridLocation);
		chunks[chunkIndex] = chunk;
	}

	ChunkType* getChunk(const glm::ivec3& chunkGridLocation) {
		uint64_t chunkIndex = getChunkIndex(chunkGridLocation);
		if (chunks.contains(chunkIndex)) {
			return &chunks.at(chunkIndex);
		}

		return nullptr;
	}

	bool isVoxelSolid(int x, int y, int z) const {
		const int ChunkSize = ChunkType::Size;

		int chunkX = (x < 0) ? (x - ChunkSize + 1) / ChunkSize : x / ChunkSize;
		int chunkY = (y < 0) ? (y - ChunkSize + 1) / ChunkSize : y / ChunkSize;
		int chunkZ = (z < 0) ? (z - ChunkSize + 1) / ChunkSize : z / ChunkSize;

		uint64_t chunkIndex = getChunkIndex(chunkX, chunkY, chunkZ);

		if (chunks.contains(chunkIndex)) {
			int xC = ((x % ChunkSize) + ChunkSize) % ChunkSize;
			int yC = ((y % ChunkSize) + ChunkSize) % ChunkSize;
			int zC = ((z % ChunkSize) + ChunkSize) % ChunkSize;
			return chunks.at(chunkIndex).isSolid(xC, yC, zC);
		}

		return false;
	}

	bool isVoxelSolid(const glm::ivec3& pos) const {
		return isVoxelSolid(pos.x, pos.y, pos.z);
	}

	uint16_t getVoxelData(int x, int y, int z) const {
		const int ChunkSize = ChunkType::Size;

		int chunkX = (x < 0) ? (x - ChunkSize + 1) / ChunkSize : x / ChunkSize;
		int chunkY = (y < 0) ? (y - ChunkSize + 1) / ChunkSize : y / ChunkSize;
		int chunkZ = (z < 0) ? (z - ChunkSize + 1) / ChunkSize : z / ChunkSize;

		uint64_t chunkIndex = getChunkIndex(chunkX, chunkY, chunkZ);

		if (chunks.contains(chunkIndex)) {
			int xC = ((x % ChunkSize) + ChunkSize) % ChunkSize;
			int yC = ((y % ChunkSize) + ChunkSize) % ChunkSize;
			int zC = ((z % ChunkSize) + ChunkSize) % ChunkSize;
			return chunks.at(chunkIndex).getVoxel(xC, yC, zC);
		}

		return std::numeric_limits<uint16_t>::max();
	}

	uint16_t getVoxelData(glm::ivec3 coords) const {
		return getVoxelData(coords.x, coords.y, coords.z);
	}

	void toggleBlock(int x, int y, int z) {
		int ChunkSize = ChunkType::Size;

		int chunkX = (x < 0) ? (x - ChunkSize + 1) / ChunkSize : x / ChunkSize;
		int chunkY = (y < 0) ? (y - ChunkSize + 1) / ChunkSize : y / ChunkSize;
		int chunkZ = (z < 0) ? (z - ChunkSize + 1) / ChunkSize : z / ChunkSize;

		uint64_t chunkIndex = getChunkIndex(chunkX, chunkY, chunkZ);

		if (chunks.contains(chunkIndex)) {
			ChunkType chunk = chunks.at(chunkIndex);
			int xC = ((x % ChunkSize) + ChunkSize) % ChunkSize;
			int yC = ((y % ChunkSize) + ChunkSize) % ChunkSize;
			int zC = ((z % ChunkSize) + ChunkSize) % ChunkSize;
			chunk.toggleBit(xC, yC, zC);
		}
	}

	char* serialize();

	static uint64_t getChunkIndex(int x, int y, int z) {
		if (x < -MAX_GRID_INT - 1 || x > MAX_GRID_INT ||
			y < -MAX_GRID_INT - 1 || y > MAX_GRID_INT ||
			z < -MAX_GRID_INT - 1 || z > MAX_GRID_INT) {
			throw std::out_of_range("Chunk coordinate out of supported range [-1048576, 1048575]");
		}

		
		uint64_t index = ((uint64_t)(z) + (MAX_GRID_INT + 1)) & 0x1FFFFF;
		index <<= 21;
		index |= ((uint64_t)(y) + (MAX_GRID_INT + 1)) & 0x1FFFFF;
		index <<= 21;
		index |= ((uint64_t)(x) + (MAX_GRID_INT + 1)) & 0x1FFFFF;

		return index;
	}

	static uint64_t getChunkIndex(const glm::ivec3& chunkGridLocation) { return getChunkIndex(chunkGridLocation.x, chunkGridLocation.y, chunkGridLocation.z); };


	static glm::ivec3 getChunkCoords(uint64_t index) {
		glm::ivec3 coords;


		coords.x = (int)((index & 0x1FFFFF) - (MAX_GRID_INT + 1));
		index >>= 21;

		coords.y = (int)((index & 0x1FFFFF) - (MAX_GRID_INT + 1));
		index >>= 21;

		coords.z = (int)((index & 0x1FFFFF) - (MAX_GRID_INT + 1));
		index >>= 21;

 		return coords;
	}


	std::unordered_map<uint64_t, ChunkType>::iterator begin() const { return chunks.begin(); }
	std::unordered_map<uint64_t, ChunkType>::iterator end() const { return chunks.end(); }

	std::unordered_map<uint64_t, ChunkType>::iterator begin() { return chunks.begin(); }
	std::unordered_map<uint64_t, ChunkType>::iterator end() { return chunks.end(); }


private:
	/* maps chunk coord to chunk data.
	 * bits 0-20 of index are for (int) x coordinate of chunk in worldspace
	 * bits 21-41 of index are for (int) y coordinate of chunk in worldspace
	 * bits 42-62 of index are for (int) z coordinate of chunk in worldspace
	 * bit 63 extra
	 * max supported world size: 2,097,152 (-1,048,576 to 1,048,576)
	*/
	std::unordered_map<uint64_t, ChunkType> chunks;
};

typedef ChunkGrid<Chunk8> ChunkGrid8;
typedef ChunkGrid<Chunk16> ChunkGrid16;
typedef ChunkGrid<Chunk32> ChunkGrid32;

#endif

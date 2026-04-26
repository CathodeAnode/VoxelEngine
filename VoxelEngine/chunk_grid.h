#ifndef CHUNKGRID_H
#define CHUNKGRID_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glfw/glfw3.h>

#include <unordered_map>
#include <stdexcept>
#include <limits>
#include <cassert>
#include <iostream>

#include "chunk.h"
#include "types.h"
#include "uid_manager.h"

#define MAX_GRID_INT 1048575

template<typename ChunkType> 
class [[deprecated("Not Updated")]] ChunkGrid
{
public:

	ChunkGrid(char* serializedData); // load from serialized data

	// generate flat world with given size
	ChunkGrid(float _voxelScale = 1.0f) 
		: m_VoxelScale(_voxelScale)
		, k_Uid(UIDManager::Generate())
	{}

	~ChunkGrid() 
	{
		m_ChunkMap.clear(); // deallocate chunks
	}

	// Copy constructor
	ChunkGrid(const ChunkGrid& other)
		: m_VoxelScale(other.m_VoxelScale)
		, k_Uid(UIDManager::Generate())
	{
		m_ChunkMap.clear();
		for (const auto& [index, chunk] : other.m_ChunkMap) 
		{
			m_ChunkMap[index] = chunk; // chunk copy constructor generates new UID
		}
	}

	// Move constructor
	ChunkGrid(ChunkGrid&& other) noexcept
		: m_VoxelScale(other.m_VoxelScale)
		, k_Uid(other.k_Uid)
	{
		m_ChunkMap.clear();
		m_ChunkMap = std::move(other.m_ChunkMap);
	}

	// Move assignment
	ChunkGrid& operator=(ChunkGrid&& other) noexcept
	{
		if (this != &other) 
		{
			m_ChunkMap.clear();
			m_ChunkMap = std::move(other.m_ChunkMap);
			m_VoxelScale = other.m_VoxelScale;
		}
		return *this;
	}

	//TODO: change to take pointers instead of making a copy of chunk (optmization)
	void addChunk(const ChunkType& chunk, const glm::ivec3& chunkGridLocation) 
	{
		uint64_t chunkIndex = EncodeChunkCoords(chunkGridLocation);
		m_ChunkMap[chunkIndex] = chunk;
	}

	ChunkType* GetChunk(const glm::ivec3& chunkGridLocation) 
	{
		uint64_t chunkIndex = EncodeChunkCoords(chunkGridLocation);
		return getChunk(chunkIndex);
	}

	ChunkType* GetChunk(uint64_t index)
	{
		if (m_ChunkMap.contains(index)) {
			return &m_ChunkMap.at(index);
		}

		return nullptr;
	}
	const ChunkType* GetChunk(const glm::ivec3& chunkGridLocation) const
	{
		uint64_t chunkIndex = EncodeChunkCoords(chunkGridLocation);
		return getChunk(chunkIndex);
	}

	const ChunkType* GetChunk(uint64_t index) const
	{
		if (m_ChunkMap.contains(index)) {
			return &m_ChunkMap.at(index);
		}

		return nullptr;
	}

	[[nodiscard]] VoxelObjectID GetChunkID(const glm::ivec3& chunkCoords) const
	{
		uint64_t index = EncodeChunkCoords(chunkCoords);

		const auto& it = m_ChunkMap.find(index);

		if (it != m_ChunkMap.end())
		{
			return it->second.GetUID();
		}

		return NULL;
	}

	bool isVoxelSolid(int x, int y, int z) const 
	{
		const int ChunkSize = ChunkType::Size;

		bool result = false;

		int chunkX = floor(x / (float)ChunkSize);
		int chunkY = floor(y / (float)ChunkSize);
		int chunkZ = floor(z / (float)ChunkSize);

		uint64_t chunkIndex = EncodeChunkCoords(chunkX, chunkY, chunkZ);

		if (m_ChunkMap.contains(chunkIndex)) {
			int xC = ((x % ChunkSize) + ChunkSize) % ChunkSize;
			int yC = ((y % ChunkSize) + ChunkSize) % ChunkSize;
			int zC = ((z % ChunkSize) + ChunkSize) % ChunkSize;
			result = m_ChunkMap.at(chunkIndex).IsSolid(xC, yC, zC);
		}


		return result;
	}

	bool isVoxelSolid(const glm::ivec3& pos) const 
	{
		return isVoxelSolid(pos.x, pos.y, pos.z);
	}

	uint16_t GetVoxelColorAt(int x, int y, int z) const 
	{
		const int ChunkSize = ChunkType::Size;

		int chunkX = floor(x / (float)ChunkSize);
		int chunkY = floor(y / (float)ChunkSize);
		int chunkZ = floor(z / (float)ChunkSize);

		uint64_t chunkIndex = EncodeChunkCoords(chunkX, chunkY, chunkZ);

		if (m_ChunkMap.contains(chunkIndex)) {
			int xC = ((x % ChunkSize) + ChunkSize) % ChunkSize;
			int yC = ((y % ChunkSize) + ChunkSize) % ChunkSize;
			int zC = ((z % ChunkSize) + ChunkSize) % ChunkSize;
			return m_ChunkMap.at(chunkIndex).getVoxel(xC, yC, zC);
		}

		return std::numeric_limits<uint16_t>::max();
	}

	uint16_t GetVoxelColorAt(glm::ivec3 coords) const 
	{
		return GetVoxelColorAt(coords.x, coords.y, coords.z);
	}

	char* serialize();

	static uint64_t EncodeChunkCoords(int x, int y, int z) 
	{
		assert(x >= -MAX_GRID_INT - 1 || x <= MAX_GRID_INT ||
			y >= -MAX_GRID_INT - 1 || y <= MAX_GRID_INT ||
			z >= -MAX_GRID_INT - 1 || z <= MAX_GRID_INT,
			"Chunk coordinate out of supported range [-1048576, 1048575]");

		
		uint64_t index = ((uint64_t)(z) + (MAX_GRID_INT + 1)) & 0x1FFFFF;
		index <<= 21;
		index |= ((uint64_t)(y) + (MAX_GRID_INT + 1)) & 0x1FFFFF;
		index <<= 21;
		index |= ((uint64_t)(x) + (MAX_GRID_INT + 1)) & 0x1FFFFF;

		return index;
	}

	inline static uint64_t EncodeChunkCoords(const glm::ivec3& chunkGridLocation) { return EncodeChunkCoords(chunkGridLocation.x, chunkGridLocation.y, chunkGridLocation.z); };


	static glm::ivec3 DecodeChunkCoords(uint64_t index) 
	{
		glm::ivec3 coords;

		coords.x = (int)((index & 0x1FFFFF) - (MAX_GRID_INT + 1));
		index >>= 21;

		coords.y = (int)((index & 0x1FFFFF) - (MAX_GRID_INT + 1));
		index >>= 21;

		coords.z = (int)((index & 0x1FFFFF) - (MAX_GRID_INT + 1));
		index >>= 21;

 		return coords;
	}

	inline float getVoxelScale() const { return m_VoxelScale; };

	inline VoxelObjectID GetUID() const { return k_Uid; };


	std::unordered_map<uint64_t, ChunkType>::iterator begin() const { return m_ChunkMap.begin(); }
	std::unordered_map<uint64_t, ChunkType>::iterator end() const { return m_ChunkMap.end(); }

	std::unordered_map<uint64_t, ChunkType>::iterator begin() { return m_ChunkMap.begin(); }
	std::unordered_map<uint64_t, ChunkType>::iterator end() { return m_ChunkMap.end(); }

private:
	/* maps chunk coord to chunk data.
	 * bits 0-20 of index are for (int) x coordinate of chunk in worldspace
	 * bits 21-41 of index are for (int) y coordinate of chunk in worldspace
	 * bits 42-62 of index are for (int) z coordinate of chunk in worldspace
	 * bit 63 extra
	 * max supported world size: 2,097,152 (-1,048,576 to 1,048,576)
	*/
	std::unordered_map<uint64_t, ChunkType> m_ChunkMap;
	float m_VoxelScale;
	const VoxelObjectID k_Uid;

	//inline static NullChunk<typename ChunkType::ValueType, ChunkType::Size> s_NullChunk;
	
};

//typedef ChunkGrid<Chunk8> ChunkGrid8;
//typedef ChunkGrid<Chunk16> ChunkGrid16;
//typedef ChunkGrid<Chunk32> ChunkGrid32;

#endif

#ifndef TERRIAN_H
#define TERRIAN_H

#include <glm/glm.hpp>
#include <memory>

#include "voxel_renderer.h"
#include "3d_ring_buffer.h"


template<typename ChunkType>
class ChunkGeneratorStrategy;

// class only handles chunk generation, chunk unload/loading & chunk pooling
// NOTE: chunk generation is injected via stratgy pattern through constructor.
// chunk generation stratgies include: no generation, flat Terrian generation, height map generation, perlin noise generation, and more if needed
template<typename ChunkType> 
class Terrian 
{
public:
	explicit Terrian(const glm::vec3& playerWorldCoords, unsigned int loadedChunksDistance, std::unique_ptr<ChunkGeneratorStrategy<ChunkType>> generator);
	// TODO: Create world from file
	Terrian(const char* filePath); 

	// NOTE: assumes player cannont move faster than one chunk per frame (will likely break if player moves faster than 1 chunk per frame)
	bool Update(const glm::vec3& playerWorldCoords);

	inline VoxelObjectID GetChunkID(const glm::ivec3& chunkCoords);

	// Using this function will mark chunk as dirty, thus saving chunk to disk
	std::shared_ptr<ChunkType> GetChunk(const glm::ivec3& chunkCoords);
	std::shared_ptr<const ChunkType> GetChunk(const glm::ivec3& chunkCoords) const;

	inline int GetLoadedChunksDistance() const { return m_LoadedChunks.GetLength(); }

	// TODO
	void SaveWorld(const char* filePath);

private:
	RingBuffer3D<std::shared_ptr<ChunkType>> m_LoadedChunks; // chunks loaded around player in distance of loadedChunksDistance/2 in box volume
	std::unique_ptr<ChunkGeneratorStrategy<ChunkType>> m_ChunkGenerator;

	glm::ivec3 m_LastPlayerGridCoords;

private:
	inline std::shared_ptr<ChunkType> _LoadChunk(const glm::ivec3& chunkCoords) const;

private:
	glm::ivec3 ToChunkGridCords(glm::vec3 worldSpaceCords) const
	{
		const int chunkSize = ChunkType::Size;
		const glm::ivec3 gridCoords(
			floor(worldSpaceCords.x / (float)chunkSize),
			floor(worldSpaceCords.y / (float)chunkSize),
			floor(worldSpaceCords.z / (float)chunkSize)
		);
		return gridCoords;
	}
};

typedef Terrian<Chunk8> Terrian8;
typedef Terrian<Chunk16> Terrian16;
typedef Terrian<Chunk32> Terrian32;

#include "terrian.tpp"

#endif

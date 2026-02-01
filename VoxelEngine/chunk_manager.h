#ifndef CHUNKMANAGER_H
#define CHUNKMANAGER_H

#include <glm/glm.hpp>

#include <memory>

#include "voxel_renderer.h"
#include "3d_ring_buffer.h"
#include "voxel_math.h"
#include "types.h"
#include "logger.h"
#include "profiler.h"



template<typename ChunkType>
class ChunkGeneratorStrategy;

// class only handles chunk unload/loading
template<typename ChunkType> 
class ChunkManager 
{
public:
	explicit ChunkManager(const glm::vec3& playerWorldCoords, unsigned int loadedChunksDistance, std::unique_ptr<ChunkGeneratorStrategy<ChunkType>> generator);
	// TODO: Create world from file
	ChunkManager(const char* filePath);

	void InitializeStartingChunks();

	// NOTE: assumes player cannont move faster than one chunk per frame (will likely break if player moves faster than 1 chunk per frame)
	bool Update(const glm::vec3& playerWorldCoords);

	inline VoxelObjectID GetChunkID(const glm::ivec3& chunkCoords);
	inline VoxelObjectID GetUID() const { return k_Uid; };

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
	const VoxelObjectID k_Uid;

private:
	inline std::shared_ptr<ChunkType> _LoadChunk(const glm::ivec3& chunkCoords) const;
};

typedef ChunkManager<Chunk8> ChunkManager8;
typedef ChunkManager<Chunk16> ChunkManager16;
typedef ChunkManager<Chunk32> ChunkManager32;

#include "chunk_manager.tpp"

#endif

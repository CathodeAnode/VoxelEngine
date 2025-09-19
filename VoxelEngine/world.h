#ifndef WORLD_H
#define WORLD_H

#include <glm/glm.hpp>

#include "chunk_grid.h"
#include "voxel_mesher.h"
#include "voxel_renderer.h"

// TEMPORARY Cache Currently set to 25mb (need testing to find optimal sizing)
// Memory overhead from renderer obj on CPU side is ~28.8kb for 25mb cache size (heap)
#define CACHE_PAGE_SIZE 50
#define CACHE_NUM_OF_PAGES 125000
#define AVERAGE_NUMBER_OF_INDIRECTCMDS_PER_CHUNK 3


// class only handles chunk generation, chunk unload/loading & chunk pooling
// NOTE: chunk generation is injected via stratgy pattern through constructor.
// chunk generation stratgies include: no generation, flat world generation, height map generation, perlin noise generation, and more if needed
template<typename ChunkType> 
class World 
{
public:
	// TODO Dependency Inject world generation stratgy obj
	World(unsigned int renderDistance);
	// TODO: Create world from file
	World(const char* filePath); 

	// TEMPORARY
	void UpdateVisibleChunksByDistance(const glm::vec3& playerWorldCoords);

	// TEMPORARY
	void Render();

	void AddChunk(const glm::ivec3& chunkCoords, const ChunkType& chunk);
	void RemoveChunk(const glm::ivec3& chunkCoords);

	inline VoxelObjectID GetChunkID(const glm::ivec3& chunkCoords);

	// Using this function will mark chunk as dirty, thus saving chunk to disk
	ChunkType* GetChunk(const glm::ivec3& chunkCoords);
	const ChunkType* GetChunk(const glm::ivec3& chunkCoords) const;

	// TODO
	void SaveWorld(const char* filePath);

	// TEMPORARY
	inline ChunkGrid<ChunkType>& getGrid() const { return const_cast<ChunkGrid<ChunkType>&>(m_Chunks); }

private:
	ChunkGrid<ChunkType> m_Chunks;
	VoxelMesher<ChunkType> m_Mesher;
	VoxelRenderer m_Renderer;

	int m_RenderDistance;
	glm::ivec3 m_LastPlayerGridCoords;
};

typedef World<Chunk8> World8;
typedef World<Chunk16> World16;
typedef World<Chunk32> World32;

#include "world.tpp"

#endif

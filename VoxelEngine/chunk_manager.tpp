#ifndef CHUNKMANAGER_TPP
#define CHUNKMANAGER_TPP
#include "chunk_manager.h"

template<typename ChunkType>
ChunkManager<ChunkType>::ChunkManager(const glm::vec3& playerWorldCoords, unsigned int loadedChunksDistance, std::unique_ptr<ChunkGeneratorStrategy<ChunkType>> generator)
	: m_LoadedChunks(loadedChunksDistance)
	, m_ChunkGenerator(std::move(generator))
	, m_LastPlayerGridCoords(INT_MAX, INT_MAX, INT_MAX)
{
	assert(loadedChunksDistance % 2 != 0, "loaded chunks distance must be odd");

	// pre-allocate all chunks in memory
	const glm::ivec3 playerGridCoords = ToChunkGridCords(playerWorldCoords);
	const int halfLoadedDist = loadedChunksDistance / 2;
	for (int z = -halfLoadedDist; z <= halfLoadedDist; ++z)
	{
		for (int y = -halfLoadedDist; y <= halfLoadedDist; ++y)
		{
			for (int x = -halfLoadedDist; x <= halfLoadedDist; ++x)
			{
				glm::ivec3 coords = playerGridCoords + glm::ivec3(x, y, z);
				m_LoadedChunks.At(coords) = _LoadChunk(coords);
			}
		}
	}
}

template<typename ChunkType>
bool ChunkManager<ChunkType>::Update(const glm::vec3& playerWorldCoords)
{
	// step 1: convert player world coordinates to grid coordinates
	const glm::ivec3 playerGridCoords = ToChunkGridCords(playerWorldCoords);

	// step 2: check if player grid coords has changed since last call
	if (playerGridCoords == m_LastPlayerGridCoords) {
		return false; //exit early
	}

	const glm::ivec3 playerGridCoordsDiff = playerGridCoords - m_LastPlayerGridCoords;

	for (int axis = 0; axis < 3; axis++)
	{
		if (playerGridCoordsDiff[axis] == 0) continue;

		const int halfLoadedDist = m_LoadedChunks.GetLength() / 2;
		int plane = halfLoadedDist * playerGridCoordsDiff[axis];

		for (int i = -halfLoadedDist; i <= halfLoadedDist; i++)
		{
			for (int j = -halfLoadedDist; j <= halfLoadedDist; j++)
			{
				glm::ivec3 local(0);
				glm::ivec3 global(0);
				switch (axis)
				{
				case 0: // x axis
					local = glm::ivec3(plane, i, j);
					break;
				case 1: // y axis
					local = glm::ivec3(i, plane, j);
					break;
				case 2: // z axis
					local = glm::ivec3(j, i, plane);
					break;
				}

				global = local + playerGridCoords;
				m_LoadedChunks.At(global) = std::move(_LoadChunk(global));
			}
		}

	}

	m_LastPlayerGridCoords = playerGridCoords;
	return true;
}

template<typename ChunkType>
VoxelObjectID ChunkManager<ChunkType>::GetChunkID(const glm::ivec3& chunkCoords)
{
	return m_LoadedChunks.GetChunkID(chunkCoords);
}

template<typename ChunkType>
std::shared_ptr<ChunkType> ChunkManager<ChunkType>::GetChunk(const glm::ivec3& chunkCoords)
{
	// TODO: mark chunk as dirty (save to world map file on disk)
	int halfLoadedDist = m_LoadedChunks.GetLength() / 2;
	if (abs(chunkCoords.x - m_LastPlayerGridCoords.x) > halfLoadedDist ||
		abs(chunkCoords.y - m_LastPlayerGridCoords.y) > halfLoadedDist ||
		abs(chunkCoords.z - m_LastPlayerGridCoords.z) > halfLoadedDist)
	{
		return _LoadChunk(chunkCoords);
	}

	return m_LoadedChunks.At(chunkCoords);
}

template<typename ChunkType>
std::shared_ptr<const ChunkType> ChunkManager<ChunkType>::GetChunk(const glm::ivec3& chunkCoords) const
{
	int halfLoadedDist = m_LoadedChunks.GetLength() / 2;
	if (abs(chunkCoords.x - m_LastPlayerGridCoords.x) > halfLoadedDist ||
		abs(chunkCoords.y - m_LastPlayerGridCoords.y) > halfLoadedDist ||
		abs(chunkCoords.z - m_LastPlayerGridCoords.z) > halfLoadedDist)
	{
		return std::static_pointer_cast<const ChunkType>(_LoadChunk(chunkCoords));
	}

	return std::static_pointer_cast<const ChunkType>(m_LoadedChunks.At(chunkCoords));
}


template<typename ChunkType>
std::shared_ptr<ChunkType> ChunkManager<ChunkType>::_LoadChunk(const glm::ivec3& chunkCoords) const
{
	// TODO: assert chunk is not already loaded in ring buffer
	// TODO: handle loading from filesystem here
	// steps:
	// 1. if chunk exisits in world map file, load and return from file
	// 2. otherwise, generate and return chunk
	std::shared_ptr<ChunkType> ret = std::make_shared<ChunkType>();
	m_ChunkGenerator->Generate(chunkCoords, ret);
	return ret;
}

#endif
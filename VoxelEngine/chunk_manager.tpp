#ifndef CHUNKMANAGER_TPP
#define CHUNKMANAGER_TPP
#include "chunk_manager.h"

template<typename ChunkType>
ChunkManager<ChunkType>::ChunkManager(const glm::vec3& playerWorldCoords, unsigned int loadedChunksDistance, std::unique_ptr<ChunkGeneratorStrategy<ChunkType>> generator)
	: m_LoadedChunks(loadedChunksDistance)
	, m_ChunkGenerator(std::move(generator))
	, m_LastPlayerGridCoords(WorldToChunk(playerWorldCoords, ChunkType::Size))
	, k_Uid(UIDManager::Generate())
{
	PROFILE_FUNCTION();
}

template<typename ChunkType>
void ChunkManager<ChunkType>::InitializeStartingChunks()
{
	PROFILE_FUNCTION();
	//assert(loadedChunksDistance % 2 != 0, "loaded chunks distance must be odd");

	// pre-allocate all chunks in memory
	const int halfLoadedDist = m_LoadedChunks.GetLength() / 2;
	for (int z = -halfLoadedDist; z <= halfLoadedDist; ++z)
	{
		for (int y = -halfLoadedDist; y <= halfLoadedDist; ++y)
		{
			for (int x = -halfLoadedDist; x <= halfLoadedDist; ++x)
			{
				glm::ivec3 coords = m_LastPlayerGridCoords + glm::ivec3(x, y, z);
				m_LoadedChunks.At(coords) = _LoadChunk(coords);
			}
		}
	}

	LOG_INFO(EngineSystem::CHUNK,
		"ChunkManager starting Chunks initialized. VoxelObjectHandle={}, PlayerChunk={}, LoadDist={}, GeneratorStrategy={}",
		k_Uid,
		glm::to_string(m_LastPlayerGridCoords),
		m_LoadedChunks.GetLength(),
		m_ChunkGenerator->ToString()
	);
}

template<typename ChunkType>
bool ChunkManager<ChunkType>::Update(const glm::vec3& playerWorldCoords)
{
	PROFILE_FUNCTION();

	// step 1: convert player world coordinates to grid coordinates
	const glm::ivec3 playerGridCoords = WorldToChunk(playerWorldCoords, ChunkType::Size);

	// step 2: check if player grid coords has changed since last call
	if (playerGridCoords == m_LastPlayerGridCoords) {
		return false; //exit early
	}

	const glm::ivec3 playerGridCoordsDiff = playerGridCoords - m_LastPlayerGridCoords;

	LOG_DEBUG(EngineSystem::CHUNK,
		"ChunkManager loading chunks in {} direction. playerGridCoords={}",
		glm::to_string(playerGridCoordsDiff),
		glm::to_string(playerGridCoords)
	);

	for (int axis = 0; axis < 3; axis++)
	{
		if (playerGridCoordsDiff[axis] == 0) continue;

		assert(abs(playerGridCoordsDiff[axis]) == 1);

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
	PROFILE_FUNCTION();

	// TODO: assert chunk is not already loaded in ring buffer
	// TODO: handle loading from filesystem here
	// TODO: log using trace tell if chunk was loaded from disk or computed
	// steps:
	// 1. if chunk exisits in world map file, load and return from file
	// 2. otherwise, generate and return chunk
	std::shared_ptr<ChunkType> ret = std::make_shared<ChunkType>();
	m_ChunkGenerator->Generate(chunkCoords, ret);
	return ret;
}

#endif
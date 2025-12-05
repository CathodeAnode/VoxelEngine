#ifndef WORLD_TPP
#define WORLD_TPP
#include "world.h"

template<typename ChunkType>
World<ChunkType>::World(const glm::vec3& playerWorldCoords, unsigned int loadedChunksDistance, std::unique_ptr<ChunkGeneratorStrategy<ChunkType>> generator)
	: m_LoadedChunks(loadedChunksDistance)
	, m_ChunkGenerator(std::move(generator))
{
	assert(loadedChunksDistance % 2 != 0, "loaded chunks distance must be odd");
	m_Renderer.Init(CACHE_NUM_OF_PAGES, CACHE_PAGE_SIZE, pow(loadedChunksDistance, 3) * AVERAGE_NUMBER_OF_INDIRECTCMDS_PER_CHUNK);

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
	UpdateRender(playerWorldCoords);
}

template<typename ChunkType>
inline void World<ChunkType>::Update(const glm::vec3& playerWorldCoords)
{
	// step 1: convert player world coordinates to grid coordinates
	const glm::ivec3 playerGridCoords = ToChunkGridCords(playerWorldCoords);

	// step 2: check if player grid coords has changed since last call
	if (playerGridCoords == m_LastPlayerGridCoords) {
		return; //exit early
	}

	const glm::ivec3 playerGridCoordsDiff = playerGridCoords - m_LastPlayerGridCoords;

	for (int axis = 0; axis < 3; axis++)
	{
		if (playerGridCoordsDiff[axis] == 0) continue;

		int plane = (m_LoadedChunks.GetLength() / 2) * playerGridCoordsDiff[axis];

		const int halfLoadedDist = m_LoadedChunks.GetLength() / 2;
		for (int i = -halfLoadedDist; i <= halfLoadedDist; i++)
		{
			for (int j = -halfLoadedDist; j <= halfLoadedDist; j++)
			{
				glm::ivec3 local(0);
				glm::ivec3 global(0);
				switch (axis)
				{
				case 0:
					local = glm::ivec3(plane, i, j);
					break;
				case 1:
					local = glm::ivec3(i, plane, j);
					break;
				case 2:
					local = glm::ivec3(j, i, plane);
					break;
				}

				global = local + playerGridCoords;
				m_LoadedChunks.At(local.x, local.y, local.z) = std::move(_LoadChunk(global));
			}
		}

	}

	UpdateRender(playerWorldCoords);
	m_LastPlayerGridCoords = playerGridCoords;
}

template<typename ChunkType>
void World<ChunkType>::UpdateRender(const glm::vec3& playerWorldCoords)
{
	const int halfLoadedDist = m_LoadedChunks.GetLength() / 2;
	for (int x = -halfLoadedDist; x <= halfLoadedDist; x++)
	{
		for (int y = -halfLoadedDist; y <= halfLoadedDist; y++)
		{
			for (int z = -halfLoadedDist; z <= halfLoadedDist; z++)
			{

				glm::ivec3 chunkCoords = ToChunkGridCords(playerWorldCoords) + glm::ivec3(x, y, z);
				std::shared_ptr<ChunkType> chunk = m_LoadedChunks.At(chunkCoords);

				if (chunk->IsEmpty()) continue;
				glm::ivec3 chunkWorldPos = chunkCoords * static_cast<int>(ChunkType::Size);


				const VoxelObjectID chunkUID = chunk->GetUID();
				if (chunkUID != NULL)
				{
					if (!m_Renderer.IsCached(chunkUID))
					{
						m_Renderer.Upload(*this, chunkCoords, m_Mesher);
					}
					m_Renderer.DrawOnNextFrame(chunkUID, chunkWorldPos);
				}

			}
		}
	}

	m_Renderer.NextFrame();
}

template<typename ChunkType>
void World<ChunkType>::Render()
{
	m_Renderer.Render();
}

template<typename ChunkType>
inline VoxelObjectID World<ChunkType>::GetChunkID(const glm::ivec3& chunkCoords)
{
	return m_LoadedChunks.GetChunkID(chunkCoords);
}

template<typename ChunkType>
std::shared_ptr<ChunkType> World<ChunkType>::GetChunk(const glm::ivec3& chunkCoords)
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
std::shared_ptr<const ChunkType> World<ChunkType>::GetChunk(const glm::ivec3& chunkCoords) const
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
inline std::shared_ptr<ChunkType> World<ChunkType>::_LoadChunk(const glm::ivec3& chunkCoords) const
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
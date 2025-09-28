#include "world.h"

template<typename ChunkType>
World<ChunkType>::World(unsigned int loadedChunksDistance)
	: m_LoadedChunks(loadedChunksDistance)
{
	assert(loadedChunksDistance % 2 != 0, "loaded chunks distance must be odd");
	m_LastPlayerGridCoords = glm::ivec3(MAX_GRID_INT, MAX_GRID_INT, MAX_GRID_INT) - 10;
	m_Renderer.Init(CACHE_NUM_OF_PAGES, CACHE_PAGE_SIZE, pow(loadedChunksDistance, 3) * AVERAGE_NUMBER_OF_INDIRECTCMDS_PER_CHUNK);
}

template<typename ChunkType>
inline void World<ChunkType>::Update(const glm::vec3& playerWorldCoords)
{
	// step 1: convert player world coordinates to grid coordinates
	const int chunkSize = ChunkType::Size;
	const glm::ivec3 playerGridCoords(
		floor(playerWorldCoords.x / (float)chunkSize),
		floor(playerWorldCoords.y / (float)chunkSize),
		floor(playerWorldCoords.z / (float)chunkSize)
	);

	// step 2: check if player grid coords has changed since last call
	if (playerGridCoords == m_LastPlayerGridCoords) {
		return; //exit early
	}

	const glm::ivec3 playerGridCoordsDiff = m_LastPlayerGridCoords - playerGridCoords;

	for (int axis = 0; axis < 3; axis)
	{
		if (playerGridCoordsDiff[axis] == 0) continue;

		int plane = playerGridCoords[axis] + static_cast<int>((m_LoadedChunks.GetLength() / 2) * playerGridCoordsDiff[axis]);

		for (int i = 0; i < m_LoadedChunks.GetLength(); i++)
		{
			for (int j = 0; j < m_LoadedChunks.GetLength(); j++)
			{
				switch (axis)
				{
				case 0:
					m_LoadedChunks.At(plane, i, j) = std::move(LoadChunk(glm::ivec3(plane, i, j)));
					break;
				case 1:
					m_LoadedChunks.At(i, plane, j) = std::move(LoadChunk(glm::ivec3(i, plane, j)));
					break;
				case 2:
					m_LoadedChunks.At(j, i, plane) = std::move(LoadChunk(glm::ivec3(j, i, plane)));
					break;
				}
			}
		}

	}
}

template<typename ChunkType>
void World<ChunkType>::UpdateRender(const glm::vec3& playerWorldCoords)
{
	// step 1: convert player world coordinates to grid coordinates
	const int chunkSize = ChunkType::Size;
	const glm::ivec3 playerGridCoords(
		floor(playerWorldCoords.x / (float)chunkSize),
		floor(playerWorldCoords.y / (float)chunkSize),
		floor(playerWorldCoords.z / (float)chunkSize)
	);

	// step 2: check if player grid coords has changed since last call
	if (playerGridCoords == m_LastPlayerGridCoords) {
		return; //exit early
	}

	//step 3: compute m_LoadedChunks to be rendered around player in sphereical volume
	const int loadedChunksDistance = m_LoadedChunks.GetLength();
	const int loadedDistRadius_2 = loadedChunksDistance * loadedChunksDistance;

	for (int x = -loadedChunksDistance; x <= loadedChunksDistance; x++)
	{
		for (int y = -loadedChunksDistance; y <= loadedChunksDistance; y++)
		{
			for (int z = -loadedChunksDistance; z <= loadedChunksDistance; z++)
			{
				if (x * x + y * y + z * z > loadedDistRadius_2) continue; // outside sphere

				glm::ivec3 chunkCoords = playerGridCoords + glm::ivec3(x, y, z); // relative to player
				ChunkType chunk = m_LoadedChunks.At(chunkCoords);

				if (chunk.IsEmpty()) continue;
				glm::ivec3 chunkWorldPos = chunkCoords * chunkSize;


				const VoxelObjectID chunkUID = chunk.GetUID();
				if (chunkUID != NULL)
				{
					if (!m_Renderer.IsCached(chunkUID))
					{
						m_Renderer.Upload(m_LoadedChunks, chunkCoords, m_Mesher);
					}
					m_Renderer.DrawOnNextFrame(chunkUID, chunkWorldPos);
				}

			}
		}
	}

	m_Renderer.NextFrame();

	m_LastPlayerGridCoords = playerGridCoords;
}

template<typename ChunkType>
void World<ChunkType>::Render()
{
	m_Renderer.Render();
}

template<typename ChunkType>
void World<ChunkType>::AddChunk(const glm::ivec3& chunkCoords, const ChunkType& chunk)
{
	//m_LoadedChunks.addChunk(chunk, chunkCoords);
}

template<typename ChunkType>
inline VoxelObjectID World<ChunkType>::GetChunkID(const glm::ivec3& chunkCoords)
{
	return m_LoadedChunks.GetChunkID(chunkCoords);
}

template<typename ChunkType>
ChunkType* World<ChunkType>::GetChunk(const glm::ivec3& chunkCoords)
{
	int halfLoadedDist = m_LoadedChunks.GetLength() / 2;
	if (abs(chunkCoords.x - m_LastPlayerGridCoords.x) > halfLoadedDist ||
		abs(chunkCoords.y - m_LastPlayerGridCoords.y) > halfLoadedDist ||
		abs(chunkCoords.z - m_LastPlayerGridCoords.z) > halfLoadedDist)
	{
		ChunkType chunk = LoadChunk(chunkCoords);
		return &chunk;
	}

	return m_LoadedChunks.At(chunkCoords);
}

template<typename ChunkType>
const ChunkType* World<ChunkType>::GetChunk(const glm::ivec3& chunkCoords) const
{
	return m_LoadedChunks.At(chunkCoords);
}


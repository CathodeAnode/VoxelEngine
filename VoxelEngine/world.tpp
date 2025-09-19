#include "world.h"

template<typename ChunkType>
World<ChunkType>::World(unsigned int renderDistance)
	: m_RenderDistance(renderDistance)
	, m_Chunks()
{
	m_LastPlayerGridCoords = glm::ivec3(MAX_GRID_INT, MAX_GRID_INT, MAX_GRID_INT) - 10;
	m_Renderer.Init(CACHE_NUM_OF_PAGES, CACHE_PAGE_SIZE, pow(m_RenderDistance, 3) * AVERAGE_NUMBER_OF_INDIRECTCMDS_PER_CHUNK);
}

template<typename ChunkType>
void World<ChunkType>::UpdateVisibleChunksByDistance(const glm::vec3& playerWorldCoords)
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


	// step 3: compute m_Chunks to be rendered around player in sphereical volume
	const int renderDistRadius_2 = m_RenderDistance * m_RenderDistance;

	for (int x = -m_RenderDistance; x <= m_RenderDistance; x++)
	{
		for (int y = -m_RenderDistance; y <= m_RenderDistance; y++)
		{
			for (int z = -m_RenderDistance; z <= m_RenderDistance; z++)
			{
				if (x * x + y * y + z * z > renderDistRadius_2) continue; // outside sphere

				glm::ivec3 chunkCoords = playerGridCoords + glm::ivec3(x, y, z); // relative to player
				uint64_t encodedChunkCoords = ChunkGrid<ChunkType>::EncodeChunkCoords(chunkCoords);
				ChunkType* chunk = m_Chunks.getChunk(encodedChunkCoords);

				if (chunk == nullptr || chunk->IsEmpty()) continue;
				glm::ivec3 chunkWorldPos = chunkCoords * chunkSize;


				const VoxelObjectID chunkUID = chunk->GetUID();
				if (chunkUID != NULL)
				{
					if (!m_Renderer.IsCached(chunkUID))
					{
						m_Renderer.Upload(m_Chunks, chunkCoords, m_Mesher);
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
	m_Chunks.addChunk(chunk, chunkCoords);
}

template<typename ChunkType>
inline VoxelObjectID World<ChunkType>::GetChunkID(const glm::ivec3& chunkCoords)
{
	return m_Chunks.GetChunkID(chunkCoords);
}

template<typename ChunkType>
ChunkType* World<ChunkType>::GetChunk(const glm::ivec3& chunkCoords)
{
	//TEMPORARY
	ChunkType* ret = m_Chunks.getChunk(chunkCoords);
	if (ret->GetUID() == NULL)
	{
		AddChunk(chunkCoords, ChunkType());
		ret = m_Chunks.getChunk(chunkCoords);
	}

	return ret;
}

template<typename ChunkType>
const ChunkType* World<ChunkType>::GetChunk(const glm::ivec3& chunkCoords) const
{
	return m_Chunks.getChunk(chunkCoords);
}


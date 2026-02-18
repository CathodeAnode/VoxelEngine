#ifndef SCENE_TPP
#define SCENE_TPP

#include "scene.h"

template<typename ChunkType>
Scene<ChunkType>::Scene(Camera& camera, ChunkManager<ChunkType>& world, VoxelRenderer<ChunkType>& renderer)
    : m_Camera(camera)
    , m_World(world)
    , m_Renderer(renderer)
	, m_WorldUpdateFlag(true)
{

}

template<typename ChunkType>
void Scene<ChunkType>::Init(unsigned int renderDistance)
{
	PROFILE_FUNCTION();

	m_RenderDist = renderDistance;

	_UploadLoadedTerrain();
	m_Renderer.DispatchFrustumCullPass(m_RenderDist, m_Camera);
	//m_Renderer.NextFrame();
	//m_Renderer.Render(m_Camera);
}

template<typename ChunkType>
void Scene<ChunkType>::Update()
{
	PROFILE_FUNCTION();

    m_Camera.Update();
	m_World.Update(m_Camera.pos);

	m_Renderer.DispatchFrustumCullPass(m_RenderDist, m_Camera);
}

template<typename ChunkType>
void Scene<ChunkType>::Render()
{
	PROFILE_FUNCTION();

	std::span<const glm::ivec4> coords = m_Renderer.GetFrustumCulledChunkCoords();

	const int chunkSize = ChunkType::Size;
	for (const auto& chunkCoord : coords)
	{
		glm::ivec3 chunkWorldPos = chunkCoord * chunkSize;
		std::shared_ptr<const ChunkType> chunk = m_World.GetChunk(chunkCoord);
		const VoxelObjectID chunkUID = chunk->GetUID();
		//if (!m_Renderer.IsCached(chunkUID))
		//{
		//	m_Renderer.Upload(m_World, chunkCoord);
		//}
		m_Renderer.DrawOnNextFrame(chunkUID, chunkWorldPos);
	}

	m_Renderer.NextFrame();
	m_Renderer.Render(m_Camera);
}

template<typename ChunkType>
inline void Scene<ChunkType>::_UploadLoadedTerrain()
{
	PROFILE_FUNCTION();

	const int halfLoadedDist = m_World.GetLoadedChunksDistance() / 2;
	const int chunkSize = ChunkType::Size;
	const glm::ivec3 cameraChunkCoords = WorldToChunk(m_Camera.pos, chunkSize);

	for (int x = -halfLoadedDist; x <= halfLoadedDist; x++)
	{
		for (int y = -halfLoadedDist; y <= halfLoadedDist; y++)
		{
			for (int z = -halfLoadedDist; z <= halfLoadedDist; z++)
			{

				glm::ivec3 chunkCoords = cameraChunkCoords + glm::ivec3(x, y, z);
				std::shared_ptr<ChunkType const> chunk = m_World.GetChunk(chunkCoords);

				if (chunk->IsEmpty()) continue;
				glm::ivec3 chunkWorldPos = chunkCoords * chunkSize;


				const VoxelObjectID chunkUID = chunk->GetUID();
				if (chunkUID != NULL_CHUNK_ID)
				{
					m_Renderer.Upload(m_World, chunkCoords);
				}

			}
		}
	}
}


/*
				const VoxelObjectID chunkUID = chunk->GetUID();
				if (chunkUID != 0)
				{
					if (!m_Renderer.IsCached(chunkUID))
					{
						m_Renderer.Upload(m_World, chunkCoords);
					}
					m_Renderer.DrawOnNextFrame(chunkUID, chunkWorldPos);
				}
*/

#endif
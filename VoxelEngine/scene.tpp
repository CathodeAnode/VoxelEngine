#ifndef SCENE_TPP
#define SCENE_TPP

#include "scene.h"

template<typename ChunkType>
Scene<ChunkType>::Scene(Camera& camera, Terrian<ChunkType>& world, VoxelRenderer<ChunkType>& renderer)
    : m_Camera(camera)
    , m_World(world)
    , m_Renderer(renderer)
{}

template<typename ChunkType>
void Scene<ChunkType>::Update()
{
    m_Camera.Update();
	m_DirtyFrame = m_World.Update(m_Camera.pos);
}


template<typename ChunkType>
void Scene<ChunkType>::Render()
{
	if (m_DirtyFrame)
	{
		_UploadTerrian();
	    m_Renderer.NextFrame();
	}
	m_Renderer.Render();
}

template<typename ChunkType>
inline void Scene<ChunkType>::_UploadTerrian()
{
	const int halfLoadedDist = m_World.GetLoadedChunksDistance() / 2;
	const int chunkSize = ChunkType::Size;
	for (int x = -halfLoadedDist; x <= halfLoadedDist; x++)
	{
		for (int y = -halfLoadedDist; y <= halfLoadedDist; y++)
		{
			for (int z = -halfLoadedDist; z <= halfLoadedDist; z++)
			{

				glm::ivec3 chunkCoords = glm::ivec3(m_Camera.pos / (float)chunkSize) + glm::ivec3(x, y, z);
				std::shared_ptr<ChunkType const> chunk = m_World.GetChunk(chunkCoords);

				if (chunk->IsEmpty()) continue;
				glm::ivec3 chunkWorldPos = chunkCoords * chunkSize;


				const VoxelObjectID chunkUID = chunk->GetUID();
				if (chunkUID != 0)
				{
					if (!m_Renderer.IsCached(chunkUID))
					{
						m_Renderer.Upload(m_World, chunkCoords);
					}
					m_Renderer.DrawOnNextFrame(chunkUID, chunkWorldPos);
				}

			}
		}
	}

}

#endif
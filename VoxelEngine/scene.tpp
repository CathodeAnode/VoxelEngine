#ifndef SCENE_TPP
#define SCENE_TPP

#include "scene.h"
#include "profiler.h"

template<typename ChunkType>
Scene<ChunkType>::Scene(ChunkManager<ChunkType>& world, VoxelRenderer<ChunkType>& renderer)
    : m_World(world)
    , m_Renderer(renderer)
{

}

template<typename ChunkType>
void Scene<ChunkType>::Init(unsigned int renderDistance, const glm::vec3& startingCameraPos)
{
	PROFILE_FUNCTION();

	m_RenderDist = renderDistance;

	_UploadLoadedTerrain(startingCameraPos);
}

template<typename ChunkType>
void Scene<ChunkType>::Update(const glm::vec3& cameraPos, int chunkMeshingTuning)
{
	PROFILE_FUNCTION();
	m_MeshChunkPerFrame += chunkMeshingTuning;

	_MeshChunks();

	m_World.Update(cameraPos);
}


template<typename ChunkType>
void Scene<ChunkType>::Render(const Camera& viewCamera, const Camera& cullCamera)
{ 
	PROFILE_FUNCTION();

	m_Renderer.Render(viewCamera);
	m_Renderer.DispatchFrustumCullPass(m_RenderDist, cullCamera); // compute frustum cullign results for this frame (frame n)
	m_Renderer.NextFrame();
}

template<typename ChunkType>
inline void Scene<ChunkType>::_UploadLoadedTerrain(const glm::vec3& cameraPos)
{
	PROFILE_FUNCTION();

	const int halfLoadedDist = m_World.GetLoadedChunksDistance() / 2;
	const int chunkSize = ChunkType::Size;
	const glm::ivec3 cameraChunkCoords = WorldToChunk(cameraPos, chunkSize);

	for (int x = -halfLoadedDist; x <= halfLoadedDist; x++)
	{
		for (int y = -halfLoadedDist; y <= halfLoadedDist; y++)
		{
			for (int z = -halfLoadedDist; z <= halfLoadedDist; z++)
			{
				
				glm::ivec3 chunkCoords = cameraChunkCoords + glm::ivec3(x, y, z);
				m_Renderer.Upload(m_World, chunkCoords);
			}
		}
	}
}

template<typename ChunkType>
void Scene<ChunkType>::_MeshChunks()
{
	std::span<const glm::ivec4> uncachedChunkCoords = m_Renderer.GetGPURequestedChunks(); // get frustum culling results from preivous frame (frame n-1)

	if (uncachedChunkCoords.empty()) return;

	//TODO: multi-thread (thread-pool)
	//TODO: schdule n chunks to be uploaded per frame rather than the entire request buffer per frame
	for (int i = 0; i < m_MeshChunkPerFrame; ++i)
	{
		m_Renderer.Upload(m_World, uncachedChunkCoords[i]);
	}
}

#endif
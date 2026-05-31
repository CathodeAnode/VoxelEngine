#ifndef SCENE_TPP
#define SCENE_TPP

#include "scene.h"
#include "profiler.h"

template<typename ChunkType>
Scene<ChunkType>::Scene(ChunkManager<ChunkType>& world, 
	VoxelRenderer<ChunkType>& renderer, VoxelEdit<ChunkType, 
	ChunkManager<ChunkType>>& voxelEdit)
    : m_World(world)
    , m_Renderer(renderer)
	, m_VoxelEdit(voxelEdit)
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
	m_UploadBudgetPerFrame += chunkMeshingTuning;
	m_RemainingUploadBudget = m_UploadBudgetPerFrame;

	_ProcessDirtyChunks();
	_UploadRequestedChunks();

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
void Scene<ChunkType>::_ProcessDirtyChunks()
{
	PROFILE_FUNCTION();

	auto dirtyChunks = m_VoxelEdit.GetDirtyChunks(m_RemainingUploadBudget);

	for (const auto& chunkCoord : dirtyChunks)
	{
		m_Renderer.Upload(m_World, chunkCoord);
	}

	const unsigned int uploadedCount = dirtyChunks.Size();

	m_RemainingUploadBudget -= uploadedCount;
}

template<typename ChunkType>
inline void Scene<ChunkType>::_UploadRequestedChunks()
{
	PROFILE_FUNCTION();

	std::span<const glm::ivec4> uncachedChunkCoords = m_Renderer.GetGPURequestedChunks(); // get frustum culling results from preivous frame (frame n-1)

	for (int i = 0; i < uncachedChunkCoords.size() && i < m_RemainingUploadBudget; ++i)
	{
		m_Renderer.Upload(m_World, glm::ivec3(uncachedChunkCoords[i]));
	}
}

#endif
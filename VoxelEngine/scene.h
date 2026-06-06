#ifndef SCENE_H
#define SCENE_H

#include <type_traits>

#include <glm/glm.hpp>

#include "chunk.h"
#include "voxel_math.h"
#include "chunk_provider_concept.h"
#include "thread_pool.h"

class Camera;

template<typename ChunkType>
class ChunkManager;

template<typename ChunkType>
class VoxelRenderer;

template<typename ChunkType, ChunkProvider<ChunkType> ChunkContainer>
class VoxelEdit;

//TODO: multi-thread (thread-pool) chunk meshing/uploading
template<typename ChunkType>
class Scene
{
public:
	Scene(unsigned int numThreads,
		ChunkManager<ChunkType>& world, 
		VoxelRenderer<ChunkType>& renderer, 
		VoxelEdit<ChunkType, ChunkManager<ChunkType>>& voxelEdit); // add DI for models/character container in the future
	
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void Init(unsigned int renderDistance, const glm::vec3& startingCameraPos);
	void Update(const glm::vec3& cameraPos, int chunkMeshingTuning);
	void Render(const Camera& viewCamera, const Camera& cullCamera);

private:
    inline void _UploadLoadedTerrain(const glm::vec3& cameraPos);
	inline void _ProcessDirtyChunks();
	inline void _UploadRequestedChunks();
	inline void _MeshNUploadChunk(const glm::ivec3& chunkCoords);

private:
	ChunkManager<ChunkType>& m_World;
	VoxelRenderer<ChunkType>& m_Renderer;
	VoxelEdit<ChunkType, ChunkManager<ChunkType>>& m_VoxelEdit;
	ThreadPool m_ThreadPool;

	unsigned int m_RenderDist = 0;
	unsigned int m_UploadBudgetPerFrame = 16;
	unsigned int m_RemainingUploadBudget = m_UploadBudgetPerFrame;

};



typedef Scene<Chunk8> Scene8;
typedef Scene<Chunk16>Scene16;
typedef Scene<Chunk32>Scene32;


#include "scene.tpp"

#endif


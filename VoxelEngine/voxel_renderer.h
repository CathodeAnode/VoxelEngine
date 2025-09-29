#ifndef VOXEL_RENDERER_H
#define VOXEL_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glfw/glfw3.h>

#include <vector>
#include <algorithm>

#include "chunk.h"
#include "chunk_grid.h"
#include "voxel_mesher.h"
#include "types.h"
#include "gpu_buffer_allocator.h"
#include "chunk_provider_concept.h"

// What happens if:
//	1. an upload causes an eviction on an object we are currently rendering or will render on next frame (bcuz of small cache size)
//		- solve by not allowing small caches (ex: min cache size 5 * maxObjectRendered)
//	2. we re-upload an object we are currently rendering

// additions needed:
//	1. rotation for each obj??
//	2. scale for each obj??

// how to?
//	1. implement a func to only update positionSSBO of current objects being rendered
//	2. support rendering of dynamic objects (currently ObjectID uses position of chunk. so how to assign an id for objects that move)


struct DrawArraysIndirectCommand 
{
	unsigned int count = 4;
	unsigned int instanceCount;
	unsigned int first = 0;
	unsigned int baseInstance;
};

template<typename ChunkType>
class VoxelRenderer 
{
public:
	VoxelRenderer();
	~VoxelRenderer();

	void Init(size_t cachePages, size_t cachePageSize, size_t renderBufferSize);

	template<ChunkProvider<ChunkType> ChunkContainer>
	void Upload(const ChunkContainer& chunkGrid, const glm::ivec3& coords, VoxelMesher<ChunkType>& mesher);
	template <ChunkProvider<ChunkType> ChunkContainer>
	void Upload(const ChunkContainer& chunkGrid, VoxelMesher<ChunkType>& mesher);

	bool UpdatePosition(VoxelObjectID objectID, const glm::vec3& newPosition);
	void DrawOnNextFrame(VoxelObjectID objectID, const glm::vec3& position);
	void NextFrame();
	void Render();

	inline bool IsCached(VoxelObjectID objectID) { return m_DataCache.Has(objectID); }
	void ToggleDrawLines();
private:
	unsigned int m_VAO, m_QuadVBO;

	GPUPagedLRUCache<VoxelQuad, VoxelObjectID> m_DataCache;
	GPUOrphanBuffer<DrawArraysIndirectCommand> m_IndirectCommandBuffer;
	GPUOrphanBuffer<glm::vec4> m_PositionSSBO;

	std::vector<VoxelObjectID> m_ObjectsRenderedInCurrentFrame;
	std::vector<VoxelObjectID> m_ObjectsRenderedInNextFrame;

	size_t m_CurrentIndirectCmdsCount = 0;
	size_t m_NextIndirectCmdsCount = 0;

	float m_QuadVertices[20] = {
		// position             texture
		0.0f,  0.0f, 0.0f,    0.0f, 1.0f,    // Bottom left
		0.0f,  1.0f, 0.0f,    1.0f, 1.0f,    // Top Left
		1.0f,  0.0f, 0.0f,    0.0f, 0.0f, // Bottom Right
		1.0f,  1.0f, 0.0f,    1.0f, 0.0f  // Top Right
	};

	bool m_DrawLines;

	const int k_TripleBuffer = 3;

private:
	// this function will clear the objects queued to be rendered on the next frame
	void _RefreshFrame();
};

// object upload cases
// case 1: object not in cache
//	- no problem just upload normally to gpu cache
// case 2: object in cache but not being rendered
//	- upload mesh normally under its own UID (same as case 1)
// case 3: object in cache and being rendered
//	- upload mesh under a temp ID then refresh frame

/*
	// use for loop for better performance
	if (std::find(m_ObjectsRenderedInCurrentFrame.begin(),
		m_ObjectsRenderedInCurrentFrame.end(),
		chunkUID) != m_ObjectsRenderedInCurrentFrame.end())
	{
		VoxelObjectID tempUID = UIDManager::Generate();
		meshWriter.SetTargetObject(tempUID);
		mesher.MeshChunk(chunkGrid, chunkCoords, meshWriter);

		m_DataCache.Swap(tempUID, chunkUID);
		_RefreshFrame();
		m_DataCache.DeallocateObject(tempUID);
	}
*/

#include "voxel_renderer.tpp"

#endif
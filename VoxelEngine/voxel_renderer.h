#ifndef VOXEL_RENDERER_H
#define VOXEL_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glfw/glfw3.h>

#include <vector>

#include "chunk.h"
#include "chunk_grid.h"
#include "voxel_mesher.h"
#include "types.h"
#include "gpu_buffer_allocator.h"

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


struct DrawArraysIndirectCommand {
	unsigned int count = 4;
	unsigned int instanceCount;
	unsigned int first = 0;
	unsigned int baseInstance;
};

class VoxelRenderer {
public:
	VoxelRenderer();
	~VoxelRenderer();

	void Init(unsigned int quadBufferSize, unsigned int maxObjectsRendered);

	template<typename ChunkType>
	void Upload(const ChunkGrid<ChunkType>& chunkGrid, const glm::ivec3& coords, const VoxelMesher<ChunkType>& mesher);

	template<typename ChunkType>
	void Upload(const ChunkGrid<ChunkType>& chunkGrid, const VoxelMesher<ChunkType>& mesher);

	bool UpdatePosition(VoxelObjectID objectID, const glm::vec3& newPosition);

	bool DrawOnNextFrame(VoxelObjectID objectID, const glm::vec3& position);

	void NextFrame();

	void ToggleDrawLines();

	/**
	 * Renders quads from data buffer with set indirect commands
	 */
	void render();


private:
	unsigned int m_VAO, m_QuadVBO;

	GPUPagedLRUCache<QuadMeshData, VoxelObjectID> m_DataCache;
	GPUOrphanBuffer<DrawArraysIndirectCommand> m_IndirectCommandBuffer;
	GPUOrphanBuffer<glm::vec4> m_PositionSSBO;

	std::vector<VoxelObjectID> m_ObjectsRenderedInCurrentFrame;
	std::vector<VoxelObjectID> m_ObjectsRenderedInNextFrame;


	float m_QuadVertices[20] = {
		// position             texture
		0.0f,  0.0f, 0.0f,    0.0f, 1.0f,    // Bottom left
		0.0f,  1.0f, 0.0f,    1.0f, 1.0f,    // Top Left
		1.0f,  0.0f, 0.0f,    0.0f, 0.0f, // Bottom Right
		1.0f,  1.0f, 0.0f,    1.0f, 0.0f  // Top Right
	};

	bool m_DrawLines;
	size_t m_MaxObjectsRendered;

	const int k_TripleBuffer = 3;

	void _UpdateFrame();
};

// object upload cases
// case 1: object not in cache
//	- no problem just upload normally to gpu cache
// case 2: object in cache but not being rendered
//	- also upload normally and old object will evenually get evicted from cache as it fills up
//	- only need to figure out a way to reclaim VoxelObjectHandle to be able to assgin for other objects
//		- answer: dont reclaim VoxelObjectHandle instead keep incrmenting until loop around back to 0
//		- bug prone if theorilically we keep VoxelObjectHandle = 1 in cache until we loop around back to 0 (solution assert cache doesnt have that VoxelObjectHandle id in cache)
// case 3: object in cache and being rendered

#endif
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

	template<typename T, unsigned int ChunkSize>
	void Upload(const Chunk<T, ChunkSize>& chunk, const VoxelMesher<Chunk<T, ChunkSize>>& mesher);

	template<typename ChunkType>
	void Upload(const ChunkGrid<ChunkType>& chunkGrid, const VoxelMesher<ChunkType>& mesher);

	void UpdatePosition(uint64_t objectID, const glm::vec3& newPosition);

	bool DrawOnNextFrame(uint64_t objectID, const glm::vec3& position);

	void NextFrame();

	void ToggleDrawLines();

	/**
	 * Renders quads from data buffer with set indirect commands
	 */
	void render();


private:
	unsigned int m_VAO, m_QuadVBO;

	GPUPagedLRUCache<QuadMeshData, uint64_t> m_DataBuffer;
	GPUOrphanBuffer<DrawArraysIndirectCommand> m_IndirectCommandBuffer;
	GPUOrphanBuffer<glm::vec4> m_PositionSSBO;


	float m_QuadVertices[20] = {
		// position             texture
		0.0f,  0.0f, 0.0f,    0.0f, 1.0f,    // Bottom left
		0.0f,  1.0f, 0.0f,    1.0f, 1.0f,    // Top Left
		1.0f,  0.0f, 0.0f,    0.0f, 0.0f, // Bottom Right
		1.0f,  1.0f, 0.0f,    1.0f, 0.0f  // Top Right
	};

	bool m_DrawLines;
	size_t m_MaxObjectsRendered;
	size_t m_ObjectsRendered = 0;



	const int k_TripleBuffer = 3;
};

#endif
#ifndef VOXEL_RENDERER_H
#define VOXEL_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glfw/glfw3.h>

#include <vector>

#include "chunk.h"
#include "chunk_grid.h"
#include "types.h"
#include "gpu_buffer_allocator.h"

struct DrawArraysIndirectCommand {
	unsigned int count = 4;
	unsigned int  instanceCount;
	unsigned int first = 0;
	unsigned int baseInstance;
};

class VoxelRenderer {
public:
	VoxelRenderer();
	~VoxelRenderer();

	void Init(unsigned int quadBufferSize, unsigned int maxObjectsRendered);

	size_t UploadMesh(const std::vector<uint32_t>& meshData);

	bool UpdateMesh(const std::vector<uint32_t>& newMeshData, const size_t& pageId);

	Page GetDataPageOffsets(const size_t& id);

	// Reserve methods
	DrawArraysIndirectCommand* GetDrawCommandsWritePtr();
	glm::vec4* GetPositionDataWritePtr();

	void CompleteBuffersWrite(size_t _objectRendererd);

	void ToggleDrawLines();

	/**
	 * Renders quads from data buffer with set indirect commands
	 */
	void render();


private:
	unsigned int VAO, quadVBO;

	GPUPagedBuffer<uint32_t> dataBuffer;
	GPUCircularBuffer<DrawArraysIndirectCommand> indirectCommandBuffer;
	GPUCircularBuffer<glm::vec4> positionSSBO;


	float quadVertices[20] = {
		// position             texture
		0.0f,  0.0f, 0.0f,    0.0f, 1.0f,    // Bottom left
		0.0f,  1.0f, 0.0f,    1.0f, 1.0f,    // Top Left
		1.0f,  0.0f, 0.0f,    0.0f, 0.0f, // Bottom Right
		1.0f,  1.0f, 0.0f,    1.0f, 0.0f  // Top Right
	};

	bool drawLines;
	size_t maxObjectsRendered;
	size_t objectsRendered = 0;
	GLsizeiptr renderHead = 0;
	void* indirectCmdsRenderHead;



	const int kTripleBuffer = 3;
};

#endif
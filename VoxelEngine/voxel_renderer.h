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


class VoxelRenderer {
public:
	/**
	 * Initializes the renderer with a scaled base quad for voxel rendering.
	 * Uploads quad data to GPU used to draw all voxels in data buffer
	 *
	 * @param quadScale Scale factor for the base quad vertices (default: 1.0f).
	 */
	VoxelRenderer();
	~VoxelRenderer();

	void Init(unsigned int quadBufferSize, unsigned int maxObjectsRendered);

	/**
	 * Overwrites the current data buffer with the provided quad data.
	 * This function uploads an array of uint32_t values, each representing an encoded voxel quads of all chunks, to the GPU buffer.
	 *
	 *
	 * @param data New quad data encoded in uint32_t, refer to ChunkQuads for encoding (array of uint32_t).
	 * @param size Length of quad data array
	 */
	void uploadData(const std::vector<uint32_t>& data);

	bool Write

	void toggleDrawLines();

	/**
	 * Renders quads from data buffer with set indirect commands
	 */
	void render();


	inline unsigned int getDataBufferSize() const { return dataBuffer.getSize(); }

private:

	struct DrawArraysIndirectCommand {
		unsigned int count = 4;
		unsigned int  instanceCount;
		unsigned int first = 0;
		unsigned int baseInstance;
	};

	unsigned int VAO, quadVBO;


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

	const int kTripleBuffer = 3;
};

#endif
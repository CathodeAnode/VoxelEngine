#ifndef VOXEL_RENDERER_H
#define VOXEL_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glfw/glfw3.h>

#include <vector>

#include "chunk.h"
#include "chunk_grid.h"
#include "types.h"

struct Page {
	unsigned int index;
	unsigned int size;
};

class VoxelRenderer {
public:
	/**
	 * Initializes the renderer with a scaled base quad for voxel rendering.
	 * Uploads quad data to GPU used to draw all voxels in data buffer
	 *
	 * @param quadScale Scale factor for the base quad vertices (default: 1.0f).
	 */
	VoxelRenderer(float quadScale = 1.0f);
	~VoxelRenderer();

	/**
	 * Allocates memory for buffers in VRAM.
	 *
	 * @param dataBufferSize Size for data buffer (in bytes).
	 * @param indirectCommandBufferSize Size for indirect command buffer (in indirect commands number).
	 * @param positionBufferSize Size for position SSBO buffer (num * glm::vec3 bytes).
	 */
	void allocBuffers(int dataBufferSize, int indirectCommandBufferSize, int positionBufferSize);

	/**
	 * Overwrites the current data buffer with the provided quad data.
	 * This function uploads an array of uint32_t values, each representing an encoded voxel quads of all chunks, to the GPU buffer.
	 *
	 *
	 * @param data New quad data encoded in uint32_t, refer to ChunkQuads for encoding (array of uint32_t).
	 * @param size Length of quad data array
	 */
	void uploadData(uint32_t* data, int size);

	/**
	 * Updates the data buffer by replacing a portion of the previous chunk/model data with new data.
	 *
	 * This function overwrites the data in the buffer between `baseIndex` and `baseIndex + oldSize`
	 * with the provided new data. It ensures that adjacent data remains intact and handles shifting of
	 * other data in the buffer as necessary to avoid overwriting.
	 *
	 * @param data New data to upload.
	 * @param baseIndex Starting index for the update.
	 * @param newSize Size of the new data (in bytes).
	 * @param oldSize Size of the old data being replaced (in bytes).
	 */
	void updateData(uint32_t* data, int index, int newSize, int oldSize);

	/**
	 * Appends new data to the end of the current data buffer on the GPU.
	 *
	 * This function adds the provided `uint32_t` quad data to the end of the existing data buffer. It
	 * ensures that the current buffer contents are not overwritten, and the new data is placed sequentially
	 * after the existing data in the buffer.
	 *
	 * @param data Pointer to an array of `uint32_t` values representing the new quad data to be appended.
	 * @param size The size (in bytes) of the data to be added. This should match the size of the `data` array.
	 */
	void addData(uint32_t* data, int size);
	void uploadIndirectCommands(std::vector<Page> indirectDrawCommands);

	void uploadPositionData(std::vector<glm::vec3> positionData);

	void toggleDrawLines();

	/**
	 * Renders quads from data buffer with set indirect commands
	 */
	void render();

private:
	int dataSize;
	int indirectCmdCount;
	unsigned int VAO, quadVBO, dataVBO, indirectCommandBuffer, positionSSBO;
	float quadVertices[20] = {
		// position             texture
		0.0f,  0.0f, 0.0f,    0.0f, 1.0f,    // Bottom left
		0.0f,  1.0f, 0.0f,    1.0f, 1.0f,    // Top Left
		1.0f,  0.0f, 0.0f,    0.0f, 0.0f, // Bottom Right
		1.0f,  1.0f, 0.0f,    1.0f, 0.0f  // Top Right
	};

	bool drawLines;

	struct DrawArraysIndirectCommand {
		unsigned int count = 4;
		unsigned int  instanceCount;
		unsigned int first = 0;
		unsigned int baseInstance;
	};
};

#endif
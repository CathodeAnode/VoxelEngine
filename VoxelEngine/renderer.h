#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glfw/glfw3.h>

#include <vector>

#include "chunk.h"
#include "voxel_geometry.h"
#include "shader.h"
#include "types.h"


class Renderer {
public:
	Renderer(Shader _shader);
	~Renderer();

	void bindGPUObjects();
	void unbindGPUObjects();

	void uploadVertexData(const float* vertexData, size_t vertexCount, size_t stride);
	void setVertexLayout(const std::vector<VertexAttribute>& attributes, size_t strides);

	/**
	 * Renders the outer layer of a chunk using conditional (if-statement) checks.
	 * Each block is drawn individually without vertex compression.
	 *
	 * Time Complexity:  O(CHUNK_SIZE^3)
	 * Space Complexity: O(1)
	 *
	 * @param chunk		The chunk to render.
	 * @param worldPos	The coordinatse of the chunk's position in world space.
	 */
	void primitiveChunkRender(const Chunk& chunk, glm::vec3 worldPos);


	void floodFillChunkRender(const Chunk& chunk, glm::vec3 worldPos);


	void binaryGreedyChunkRender(const Chunk& chunk, glm::vec3 worldPos);

	void drawVertexMesh(size_t vertexCount, glm::vec3 worldPos, glm::vec3 scale);

private:
	unsigned int VAO, VBO, EBO;
	Shader shader;

	void drawVoxelFace(VoxelFace face, glm::vec3 worldPos) {
		const auto& faceVertices = VoxelGeometry::getFaceVertices(face);
		const auto& layout = VoxelGeometry::getVertexLayout();
		size_t stride = VoxelGeometry::getVertexStride();

		uploadVertexData(faceVertices.data(), 6, stride); // 6 vertices per face
		setVertexLayout(layout, stride);

		drawVertexMesh(6, worldPos, glm::vec3(1.0f)); // Assume uniform scale
	}
};


/*
	Renderer renderer(myShader);
	renderer.bindGPUObjects();

	std::vector<float> myMesh = { };
	size_t vertexStride = 6 * sizeof(float); // e.g., vec3 position + vec3 normal

	std::vector<VertexAttribute> attributes = {
		{0, 3, GL_FLOAT, GL_FALSE, 0},                       // Position
		{1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float)}        // Normal
	};

	renderer.uploadVertexData(myMesh.data(), myMesh.size() / 6, vertexStride);
	renderer.setVertexLayout(attributes, vertexStride);

	// In draw loop
	renderer.drawVertices(myMesh.size() / 6);
*/

#endif
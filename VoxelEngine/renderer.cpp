#include "renderer.h"

Renderer::Renderer(Shader _shader) {
    shader = _shader;
}

Renderer::~Renderer() {

}

void Renderer::bindGPUObjects() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	//glGenBuffers(1, &EBO);
}

void Renderer::unbindGPUObjects() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    //glDeleteBuffers(1, &EBO);
}

void Renderer::setVertexLayout(const std::vector<VertexAttribute>& attributes, size_t stride) {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    for (const auto& attr : attributes) {
        glVertexAttribPointer(attr.index, attr.size, attr.type, attr.normalized, stride, (void*)(uintptr_t)attr.offset);
        glEnableVertexAttribArray(attr.index);
    }
}

void Renderer::uploadVertexData(const float* vertexData, size_t vertexCount, size_t stride) {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * stride, vertexData, GL_STATIC_DRAW);
}

void Renderer::drawVertexMesh(size_t vertexCount, glm::vec3 worldPos, glm::vec3 scale) {
    shader.use();
    glBindVertexArray(VAO);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, worldPos);
    model = glm::scale(model, scale);
    shader.setMat4("model", model);

    glDrawArrays(GL_LINE_STRIP, 0, vertexCount); // maybe add func param for GL_TRIANGLES, GL_LINE_STRIP
    glBindVertexArray(0);
}

void Renderer::primitiveChunkRender(const Chunk& chunk, glm::vec3 worldPos) {
    unsigned int size = chunk.getChunkSize();
    glm::vec3 chunkPos;

    auto isSolid = [&](int x, int y, int z) -> bool {
        if (x < 0 || x >= size || y < 0 || y >= size || z < 0 || z >= size)
            return false;
        int index = z * size * size + y * size + x;
        return chunk.data[index] != 0;
    };

    for (int z = 0; z < size; ++z) {
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                int index = z * (size * size) + y * size + x;

                if (chunk.data[index] == 0) continue;

                chunkPos = glm::vec3(worldPos.x + x, worldPos.y + y, worldPos.z + z);
                
                // For each face, check if adjacent voxel is empty or out of bounds
                if (!isSolid(x - 1, y, z)) {
                    // render -X face
                    drawVoxelFace(VoxelFace::Left, chunkPos);
                }
                if (!isSolid(x + 1, y, z)) {
                    // render +X face
                    drawVoxelFace(VoxelFace::Right, chunkPos);
                }
                if (!isSolid(x, y - 1, z)) {
                    // render -Y face
                    drawVoxelFace(VoxelFace::Down, chunkPos);
                }
                if (!isSolid(x, y + 1, z)) {
                    // render +Y face
                    drawVoxelFace(VoxelFace::Up, chunkPos);
                }
                if (!isSolid(x, y, z - 1)) {
                    // render -Z face
                    drawVoxelFace(VoxelFace::Front, chunkPos - glm::vec3(0, 0, 1));
                }
                if (!isSolid(x, y, z + 1)) {
                    // render +Z face
                    drawVoxelFace(VoxelFace::Back, chunkPos + glm::vec3(0, 0, 1));
                }

            }
        }
    }
}
void Renderer::floodFillChunkRender(const Chunk& chunk, glm::vec3 worldPos) {

}
void Renderer::binaryGreedyChunkRender(const Chunk& chunk, glm::vec3 worldPos) {

}
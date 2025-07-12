#include "voxel_renderer.h"


VoxelRenderer::VoxelRenderer(float quadScale) 
{
    drawLines = false;
    dataSize = 0;
    indirectCmdCount = 0;

    for (int i = 0; i < 4; i += 5) { 
        quadVertices[i] *= quadScale;
        quadVertices[i + 1] *= quadScale;
        quadVertices[i + 2] *= quadScale;
    }
    // setup default quad that will be instanced
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);


    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    // TexCoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

VoxelRenderer::~VoxelRenderer() 
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &quadVBO);
}

void VoxelRenderer::allocBuffers(int dataBufferSize, int indirectCommandBufferSize, int positionBufferSize)
{
    glGenBuffers(1, &dataVBO);
    glBindBuffer(GL_ARRAY_BUFFER, dataVBO);
    glBufferData(GL_ARRAY_BUFFER, dataBufferSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(VAO);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, dataVBO);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(uint32_t), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(2, 1);
    glBindVertexArray(0);


    glGenBuffers(1, &indirectCommandBuffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectCommandBuffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, indirectCommandBufferSize * sizeof(DrawArraysIndirectCommand), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    glGenBuffers(1, &positionSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, positionSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, positionBufferSize * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, positionSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void VoxelRenderer::uploadData(uint32_t* data, int size)
{
    //for (int i = 0; i < size; ++i) {
    //    std::cout << "  data[" << i << "] = 0x"
    //        << std::hex << data[i] << std::dec
    //        << " (" << data[i] << ")\n";
    //}

    dataSize = size;
    glBindBuffer(GL_ARRAY_BUFFER, dataVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size * sizeof(uint32_t), data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VoxelRenderer::updateData(uint32_t* data, int index, int newSize, int oldSize)
{
    glBindBuffer(GL_ARRAY_BUFFER, dataVBO);
    // shift data in buffer to make room for new data
    if (newSize > oldSize) {
        glCopyBufferSubData(dataVBO, dataVBO, index + oldSize, index + newSize, dataSize - index + oldSize);
    }
    if (newSize < oldSize) {
        glCopyBufferSubData(dataVBO, dataVBO, index + oldSize, index + (oldSize - newSize), dataSize - index + oldSize);
    }

    glBufferSubData(GL_ARRAY_BUFFER, index, newSize, data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VoxelRenderer::addData(uint32_t* data, int size)
{
    glBindBuffer(GL_ARRAY_BUFFER, dataVBO);
    glBufferSubData(GL_ARRAY_BUFFER, dataSize, size, data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    dataSize += size;
}

void VoxelRenderer::uploadIndirectCommands(std::vector<Page> indirectDrawCommands)
{
    std::vector<DrawArraysIndirectCommand> drawCommands;
    indirectCmdCount = indirectDrawCommands.size();
    drawCommands.resize(indirectCmdCount);

    DrawArraysIndirectCommand cmd;
    for (int i = 0; i < indirectDrawCommands.size(); i++) {
        //cmd.count = 4; 
        //cmd.first = 0;
        cmd.baseInstance = indirectDrawCommands[i].index;
        cmd.instanceCount = indirectDrawCommands[i].size;

        std::cout << cmd.first << " " << cmd.count << " " << cmd.baseInstance << " " << cmd.instanceCount << std::endl;

        drawCommands[i] = cmd;
    }

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectCommandBuffer);
    glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, drawCommands.size() * sizeof(DrawArraysIndirectCommand), drawCommands.data());
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void VoxelRenderer::uploadPositionData(std::vector<glm::vec3> positionData)
{
    int size = positionData.size();
    std::vector<glm::vec4> paddedPositionData;
    paddedPositionData.resize(size);
    for (int i = 0; i < size; i++) {
        paddedPositionData[i] = glm::vec4(positionData[i].x, positionData[i].y, positionData[i].z, 0.0f);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, positionSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size * sizeof(glm::vec4), paddedPositionData.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

}


void VoxelRenderer::toggleDrawLines()
{
    drawLines = !drawLines;
}


void VoxelRenderer::render() 
{
    glBindVertexArray(VAO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, positionSSBO);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectCommandBuffer);

    if (drawLines) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, nullptr, indirectCmdCount, 0);
    //glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, 2, 131);
    //glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, 2, 153);

    if (drawLines) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindVertexArray(0);
}


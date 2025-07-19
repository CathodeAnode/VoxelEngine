#include "voxel_renderer.h"


VoxelRenderer::VoxelRenderer()
    : indirectCommandBuffer(true)
    , positionSSBO(true)
    , drawLines(false)
{
}

VoxelRenderer::~VoxelRenderer() 
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &quadVBO);
}

void VoxelRenderer::Init(unsigned int quadBufferSize, unsigned int maxObjectsRendered)
{
    indirectCommandBuffer.Create(GL_DRAW_INDIRECT_BUFFER, kTripleBuffer * maxObjectsRendered);
    positionSSBO.Create(GL_SHADER_STORAGE_BUFFER, kTripleBuffer * maxObjectsRendered);

    //for (int i = 0; i < 4; i += 5) { 
    //    quadVertices[i] *= quadScale;
    //    quadVertices[i + 1] *= quadScale;
    //    quadVertices[i + 2] *= quadScale;
    //}



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

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, dataBuffer.getBufferID());
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(uint32_t), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(2, 1);
    glBindVertexArray(0);
}

void VoxelRenderer::uploadData(const std::vector<uint32_t>& data)
{
    dataBuffer.upload(data);
}

void VoxelRenderer::updateData(const std::vector<uint32_t>& data, int index, int oldSize)
{
    dataBuffer.replace(data, index, oldSize);
}

void VoxelRenderer::addData(const std::vector<uint32_t>& data) 
{
    dataBuffer.append(data);
}

void VoxelRenderer::uploadIndirectCommands(const std::vector<Page>& indirectDrawCommands)
{
    std::vector<DrawArraysIndirectCommand> drawCommands;
    unsigned int indirectCmdCount = indirectDrawCommands.size();
    drawCommands.resize(indirectCmdCount);

    DrawArraysIndirectCommand cmd;
    for (unsigned int i = 0; i < indirectDrawCommands.size(); i++) {
        //cmd.count = 4; 
        //cmd.first = 0;
        cmd.baseInstance = indirectDrawCommands[i].index;
        cmd.instanceCount = indirectDrawCommands[i].size;

        //std::cout << cmd.first << " " << cmd.count << " " << cmd.baseInstance << " " << cmd.instanceCount << std::endl;

        drawCommands[i] = cmd;
    }

    indirectCommandBuffer.upload(drawCommands);
}

void VoxelRenderer::uploadPositionData(const std::vector<glm::vec3>& positionData)
{
    int size = positionData.size();
    std::vector<glm::vec4> paddedPositionData;
    paddedPositionData.resize(size);
    for (int i = 0; i < size; i++) {
        paddedPositionData[i] = glm::vec4(positionData[i].x, positionData[i].y, positionData[i].z, 0.0f);
    }

    positionSSBO.upload(paddedPositionData);

}


void VoxelRenderer::toggleDrawLines()
{
    drawLines = !drawLines;
}


void VoxelRenderer::render() 
{
    glBindVertexArray(VAO);


    if (drawLines) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, indirectCommandBuffer.GetHeadOffset(), , 0);
    //glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, 2, 131);
    //glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, 2, 153);

    if (drawLines) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    indirectCommandBuffer.OnUsageComplete();
    positionSSBO.OnUsageComplete()
    glBindVertexArray(0);
}


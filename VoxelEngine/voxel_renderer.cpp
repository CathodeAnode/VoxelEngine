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

void VoxelRenderer::Init(unsigned int _quadBufferSize, unsigned int _maxObjectsRendered)
{
    maxObjectsRendered = _maxObjectsRendered;

    dataBuffer.Create(GL_ARRAY_BUFFER, _quadBufferSize);
    indirectCommandBuffer.Create(GL_DRAW_INDIRECT_BUFFER, kTripleBuffer * _maxObjectsRendered);
    positionSSBO.Create(GL_SHADER_STORAGE_BUFFER, kTripleBuffer * _maxObjectsRendered);

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
    glBindBuffer(GL_ARRAY_BUFFER, dataBuffer.GetName());
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(uint32_t), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(2, 1);
    glBindVertexArray(0);
}

size_t VoxelRenderer::UploadMesh(const std::vector<uint32_t>& meshData)
{
    return dataBuffer.UploadPageData(meshData);
}

bool VoxelRenderer::UpdateMesh(const std::vector<uint32_t>& newMeshData, const size_t& pageId)
{
    return dataBuffer.UpdatePage(pageId, newMeshData);
}

Page VoxelRenderer::GetDataPageOffsets(const size_t& id)
{
    return dataBuffer.GetPageOffset(id);
}

DrawArraysIndirectCommand* VoxelRenderer::GetDrawCommandsWritePtr()
{
    return indirectCommandBuffer.Reserve(maxObjectsRendered);
}

glm::vec4* VoxelRenderer::GetPositionDataWritePtr()
{
    return positionSSBO.Reserve(maxObjectsRendered);
}

void VoxelRenderer::CompleteBuffersWrite(size_t _objectRendererd)
{
    indirectCmdsRenderHead = indirectCommandBuffer.GetHeadOffset();
    renderHead = positionSSBO.OnUsageComplete(maxObjectsRendered);
    indirectCommandBuffer.OnUsageComplete(maxObjectsRendered);
    objectsRendered = _objectRendererd;
}

void VoxelRenderer::ToggleDrawLines()
{
    drawLines = !drawLines;
}


void VoxelRenderer::render() 
{
    if (objectsRendered == 0) return;

    glBindVertexArray(VAO);
    positionSSBO.BindBufferRange(0, renderHead, objectsRendered);

    if (drawLines) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, indirectCmdsRenderHead, objectsRendered, 0);

    if (drawLines) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBindVertexArray(0);

}


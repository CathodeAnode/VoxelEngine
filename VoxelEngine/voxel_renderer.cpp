#include "voxel_renderer.h"


VoxelRenderer::VoxelRenderer()
    : m_IndirectCommandBuffer(true)
    , m_PositionSSBO(true)
    , m_DrawLines(false)
{
}

VoxelRenderer::~VoxelRenderer() 
{
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_QuadVBO);
}

void VoxelRenderer::Init(unsigned int _quadBufferSize, unsigned int _maxObjectsRendered)
{
    m_MaxObjectsRendered = _maxObjectsRendered;

    m_DataBuffer.Create(GL_ARRAY_BUFFER, _quadBufferSize);
    m_IndirectCommandBuffer.Create(GL_DRAW_INDIRECT_BUFFER, kTripleBuffer * _maxObjectsRendered);
    m_PositionSSBO.Create(GL_SHADER_STORAGE_BUFFER, kTripleBuffer * _maxObjectsRendered);

    //for (int i = 0; i < 4; i += 5) { 
    //    m_QuadVertices[i] *= quadScale;
    //    m_QuadVertices[i + 1] *= quadScale;
    //    m_QuadVertices[i + 2] *= quadScale;
    //}



// setup default quad that will be instanced
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    glGenBuffers(1, &m_QuadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);


    glBufferData(GL_ARRAY_BUFFER, sizeof(m_QuadVertices), m_QuadVertices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    // TexCoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, m_DataBuffer.GetName());
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(uint32_t), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(2, 1);
    glBindVertexArray(0);
}

size_t VoxelRenderer::UploadMesh(const std::vector<uint32_t>& meshData)
{
    return m_DataBuffer.UploadPageData(meshData);
}

bool VoxelRenderer::UpdateMesh(const std::vector<uint32_t>& newMeshData, const size_t& pageId)
{
    return m_DataBuffer.UpdatePage(pageId, newMeshData);
}

Page VoxelRenderer::GetDataPageOffsets(const size_t& m_Id)
{
    return m_DataBuffer.GetPageOffset(m_Id);
}

DrawArraysIndirectCommand* VoxelRenderer::GetDrawCommandsWritePtr()
{
    return m_IndirectCommandBuffer.Reserve(m_MaxObjectsRendered);
}

glm::vec4* VoxelRenderer::GetPositionDataWritePtr()
{
    return m_PositionSSBO.Reserve(m_MaxObjectsRendered);
}

void VoxelRenderer::CompleteBuffersWrite(size_t _objectRendererd)
{
    m_IndirectCmdsRenderHead = m_IndirectCommandBuffer.GetHeadOffset();
    m_RenderHead = m_PositionSSBO.OnUsageComplete(m_MaxObjectsRendered);
    m_IndirectCommandBuffer.OnUsageComplete(m_MaxObjectsRendered);
    m_ObjectsRendered = _objectRendererd;
}

void VoxelRenderer::ToggleDrawLines()
{
    m_DrawLines = !m_DrawLines;
}


void VoxelRenderer::render() 
{
    if (m_ObjectsRendered == 0) return;

    glBindVertexArray(m_VAO);
    m_PositionSSBO.BindBufferRange(0, m_RenderHead, m_ObjectsRendered);

    if (m_DrawLines) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, m_IndirectCmdsRenderHead, m_ObjectsRendered, 0);

    if (m_DrawLines) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBindVertexArray(0);

}


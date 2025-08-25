#include "voxel_renderer.h"


VoxelRenderer::VoxelRenderer()
    : m_DataCache(true)
    , m_IndirectCommandBuffer(true)
    , m_PositionSSBO(true)
    , m_DrawLines(false)
{}

VoxelRenderer::~VoxelRenderer() 
{
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_QuadVBO);
}

void VoxelRenderer::Init(size_t cachePages, size_t cachePageSize, size_t indirectBufferSize)
{
    assert(cachePages * cachePageSize * 5 > indirectBufferSize, "Cache Size too small");

    m_IndirectCommandBuffer.Create(GL_DRAW_INDIRECT_BUFFER, indirectBufferSize, k_TripleBuffer);
    m_PositionSSBO.Create(GL_SHADER_STORAGE_BUFFER, indirectBufferSize, k_TripleBuffer);
    m_DataCache.Create(GL_ARRAY_BUFFER, cachePageSize, cachePages);

    m_ObjectsRenderedInCurrentFrame.reserve(indirectBufferSize);
    m_ObjectsRenderedInNextFrame.reserve(indirectBufferSize);

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
    glBindBuffer(GL_ARRAY_BUFFER, m_DataCache.GetName());
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(uint32_t), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(2, 1);
    glBindVertexArray(0);
}

bool VoxelRenderer::DrawOnNextFrame(VoxelObjectID objectID, const glm::vec3& position)
{
    if(!m_DataCache.Has(objectID))
        return false;

    std::vector<GPUBufferRange> memoryRanges = m_DataCache.GetObjectBufferRanges(objectID);
    DrawArraysIndirectCommand* cmds = m_IndirectCommandBuffer.GetHeadContents();
    glm::vec4* paddedPos = m_PositionSSBO.GetHeadContents();

    for (const auto& range : memoryRanges)
    {
        cmds->count = 4;
        cmds->first = 0;
        cmds->baseInstance = range.startOffset;
        cmds->instanceCount = range.length;

        paddedPos->x = position.x;
        paddedPos->y = position.y;
        paddedPos->z = position.z;
        paddedPos->w = 0;

        cmds++;
        paddedPos++;
    }

    m_NextIndirectCmdsCount += memoryRanges.size();
    m_ObjectsRenderedInNextFrame.push_back(objectID);


    return true;
}

void VoxelRenderer::NextFrame()
{
    m_ObjectsRenderedInNextFrame.swap(m_ObjectsRenderedInCurrentFrame);
    std::swap(m_CurrentIndirectCmdsCount, m_NextIndirectCmdsCount);

    m_IndirectCommandBuffer.AdvanceHead();
    m_PositionSSBO.AdvanceHead();

    m_IndirectCommandBuffer.AdvanceTail();
    m_PositionSSBO.AdvanceTail();
}

void VoxelRenderer::ToggleDrawLines()
{
    m_DrawLines = !m_DrawLines;
}


void VoxelRenderer::render() 
{
    glBindVertexArray(m_VAO);
    m_PositionSSBO.BindTailBufferRange(m_CurrentIndirectCmdsCount);

    if (m_DrawLines) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, m_IndirectCommandBuffer.GetHeadOffset(), m_CurrentIndirectCmdsCount, 0);

    if (m_DrawLines) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBindVertexArray(0);

}

void VoxelRenderer::_RefreshFrame()
{
}

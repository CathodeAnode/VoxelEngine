#include "voxel_renderer.h"


VoxelRenderer::VoxelRenderer()
    : m_DataCache(true)
    , m_IndirectCommandBuffer(true)
    , m_PositionSSBO(true)
    , m_DrawLines(true)
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
    
    m_IndirectCommandBuffer.AdvanceHead();
    m_PositionSSBO.AdvanceHead();

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

void VoxelRenderer::DrawOnNextFrame(VoxelObjectID objectID, const glm::vec3& position)
{
    assert(m_DataCache.Has(objectID), "Error: Object must be uploaded before drawing");
    
    std::vector<GPUBufferRange> memoryRanges = m_DataCache.GetObjectBufferRanges(objectID);
    DrawArraysIndirectCommand* cmds = m_IndirectCommandBuffer.GetHeadContents() + m_NextIndirectCmdsCount;
    glm::vec4* paddedPos = m_PositionSSBO.GetHeadContents() + m_NextIndirectCmdsCount;

    // optimization can be done here if we can guarantee object buffer ranges wont be defragmented
    // but that would require completely changing the archtecture of the GPUcache object lol
    // having one range for each object is better bcuz we would only have one indirect cmd per object
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
}

void VoxelRenderer::NextFrame()
{
    if (m_NextIndirectCmdsCount == 0) return;

    m_ObjectsRenderedInNextFrame.swap(m_ObjectsRenderedInCurrentFrame);

    m_IndirectCommandBuffer.AdvanceHead();
    m_PositionSSBO.AdvanceHead();

    m_IndirectCommandBuffer.AdvanceTail();
    m_PositionSSBO.AdvanceTail();

    m_CurrentIndirectCmdsCount = m_NextIndirectCmdsCount;
    m_NextIndirectCmdsCount = 0;
}

void VoxelRenderer::ToggleDrawLines()
{
    m_DrawLines = !m_DrawLines;
}


void VoxelRenderer::Render() 
{
    glBindVertexArray(m_VAO);
    m_PositionSSBO.BindTailBufferRange(m_CurrentIndirectCmdsCount);
    //assert(glGetError() == GL_NO_ERROR);

    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_PositionSSBO.GetName());

    if (m_DrawLines) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, m_IndirectCommandBuffer.GetTailOffset(), m_CurrentIndirectCmdsCount, 0);
    //assert(glGetError() == GL_NO_ERROR);

    if (m_DrawLines) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBindVertexArray(0);

}

void VoxelRenderer::_RefreshFrame()
{
}

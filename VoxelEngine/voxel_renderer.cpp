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

void VoxelRenderer::Init(size_t cachePages, size_t cachePageSize, size_t renderBufferSize)
{
    assert(cachePages * cachePageSize * 5 > renderBufferSize, "Cache Size too small");

    m_IndirectCommandBuffer.Create(GL_DRAW_INDIRECT_BUFFER, renderBufferSize, k_TripleBuffer);
    m_PositionSSBO.Create(GL_SHADER_STORAGE_BUFFER, renderBufferSize, k_TripleBuffer);
    m_DataCache.Create(GL_ARRAY_BUFFER, cachePageSize, cachePages);

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

template<typename ChunkType>
inline void VoxelRenderer::Upload(const ChunkGrid<ChunkType>& chunkGrid, const glm::ivec3& coords, const VoxelMesher<ChunkType>& mesher)
{
    //ChunkType* chunk = 

    //if(m_DataCache.Has())
}


void VoxelRenderer::ToggleDrawLines()
{
    m_DrawLines = !m_DrawLines;
}


void VoxelRenderer::render() 
{
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

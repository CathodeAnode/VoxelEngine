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

template<typename ChunkType>
inline void VoxelRenderer::Upload(const ChunkGrid<ChunkType>& chunkGrid, const glm::ivec3& chunkCoords, const VoxelMesher<ChunkType>& mesher)
{
    ChunkType* chunk = chunkGrid.getChunk(chunkCoords);
    VoxelObjectID chunkUID = chunk->GetUid();
    GPUVoxelMeshCacheWriter meshWriter(m_DataCache);


    if (std::find(m_ObjectsRenderedInCurrentFrame.begin(), 
                  m_ObjectsRenderedInCurrentFrame.end(), 
                  chunkUID) != m_ObjectsRenderedInCurrentFrame.end())
    {
        VoxelObjectID tempUID = UIDManager::Generate();
        meshWriter.SetTargetObject(tempUID);
        mesher.MeshChunk(chunkGrid, chunkCoords, meshWriter);

        m_DataCache.Swap(tempUID, chunkUID);
        _RefreshFrame();
        m_DataCache.DeallocateObject(tempUID);
    }
    else
    {
        meshWriter.SetTargetObject(chunkUID);
        mesher.MeshChunk(chunkGrid, chunkCoords, meshWriter);
    }
}

template<typename ChunkType>
void VoxelRenderer::Upload(const ChunkGrid<ChunkType>& chunkGrid, const VoxelMesher<ChunkType>& mesher)
{
    VoxelObjectID gridUID = chunkGrid.GetUid();
    GPUVoxelMeshCacheWriter meshWriter(m_DataCache);

    if (std::find(m_ObjectsRenderedInCurrentFrame.begin(),
        m_ObjectsRenderedInCurrentFrame.end(),
        gridUID) != m_ObjectsRenderedInCurrentFrame.end())
    {
        VoxelObjectID tempUID = UIDManager::Generate();
        meshWriter.SetTargetObject(tempUID);
        mesher.MeshChunkGrid(chunkGrid, meshWriter);

        m_DataCache.Swap(tempUID, gridUID);
        _RefreshFrame();
        m_DataCache.DeallocateObject(tempUID);
    }
    else
    {
        meshWriter.SetTargetObject(gridUID);
        mesher.MeshChunkGrid(chunkGrid, meshWriter);
    }
}


bool VoxelRenderer::DrawOnNextFrame(VoxelObjectID objectID, const glm::vec3& position)
{
    if(!m_DataCache.Has(objectID))
        return false;

    std::vector<GPUBufferRange> memoryRanges = m_DataCache.GetObjectBufferRanges(objectID);


    return true;
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

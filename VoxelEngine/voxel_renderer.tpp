#ifndef VOXEL_RENDERER_TPP
#define VOXEL_RENDERER_TPP
#include "voxel_renderer.h"

template <typename ChunkType>
VoxelRenderer<ChunkType>::VoxelRenderer(std::unique_ptr<VoxelMesher<ChunkType>> mesher)
    : m_DataCache(true)
    , m_IndirectCommandBuffer(true)
    , m_PositionSSBO(true)
    , m_DrawLines(false)
    , m_Mesher(std::move(mesher))
    , m_VoxelShaders({{"voxel_shader.vert.glsl", GL_VERTEX_SHADER}, {"voxel_shader.frag.glsl", GL_FRAGMENT_SHADER}})
{
}

template <typename ChunkType>
VoxelRenderer<ChunkType>::~VoxelRenderer() 
{
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_QuadVBO);
}

template <typename ChunkType>
void VoxelRenderer<ChunkType>::Init(size_t cachePages, size_t cachePageSize, size_t indirectBufferSize)
{
    assert(cachePages * cachePageSize * 5 > indirectBufferSize, "Cache Size too small");

    glEnable(GL_DEPTH_TEST);

    m_IndirectCommandBuffer.Create(GL_DRAW_INDIRECT_BUFFER, indirectBufferSize, k_TripleBuffer);
    m_PositionSSBO.Create(GL_SHADER_STORAGE_BUFFER, indirectBufferSize, k_TripleBuffer);
    m_DataCache.Create(GL_ARRAY_BUFFER, cachePageSize, cachePages);
    
    // Offset head from tail on OrphanBuffers
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

    glBindBuffer(GL_ARRAY_BUFFER, m_DataCache.GetName());

    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(VoxelQuad), (void*)offsetof(VoxelQuad, data));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(VoxelQuad), (void*)offsetof(VoxelQuad, color));
    glVertexAttribDivisor(3, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

template<typename ChunkType>
template <ChunkProvider<ChunkType> ChunkContainer>
void VoxelRenderer<ChunkType>::Upload(const ChunkContainer& chunkContainer, const glm::ivec3& chunkCoords)
{
    std::shared_ptr<const ChunkType> chunk = chunkContainer.GetChunk(chunkCoords);
    VoxelObjectID chunkUID = chunk->GetUID();
    GPUVoxelMeshCacheWriter meshWriter(m_DataCache);

    if (IsCached(chunkUID))
    {
        VoxelObjectID tempUID = UIDManager::Generate();
        meshWriter.SetTargetObject(tempUID);
        m_Mesher->MeshChunk(chunkContainer, chunkCoords, meshWriter);

        m_DataCache.Swap(tempUID, chunkUID);
        //_RefreshFrame();
        m_DataCache.DeallocateObject(tempUID);
    }
    else
    {
        meshWriter.SetTargetObject(chunkUID);
        m_Mesher->MeshChunk(chunkContainer, chunkCoords, meshWriter);
    }
}

template<typename ChunkType>
template <ChunkProvider<ChunkType> ChunkContainer>
void VoxelRenderer<ChunkType>::Upload(const ChunkContainer& chunkContainer)
{
    VoxelObjectID gridUID = chunkContainer.GetUID();
    GPUVoxelMeshCacheWriter meshWriter(m_DataCache);

    if (IsCached(gridUID))
    {
        VoxelObjectID tempUID = UIDManager::Generate();
        meshWriter.SetTargetObject(tempUID);
        m_Mesher->MeshChunk(chunkContainer, meshWriter);

        m_DataCache.Swap(tempUID, gridUID);
        //_RefreshFrame();
        m_DataCache.DeallocateObject(tempUID);
    }
    else
    {
        meshWriter.SetTargetObject(gridUID);
        m_Mesher->MeshChunkGrid(chunkContainer, meshWriter);
    }

}

template <typename ChunkType>
void VoxelRenderer<ChunkType>::DrawOnNextFrame(VoxelObjectID objectID, const glm::vec3& position)
{
    if (!m_DataCache.Has(objectID)) return;
    
    std::vector<GPUBufferRange> memoryRanges = m_DataCache.GetObjectBufferRanges(objectID);
    DrawArraysIndirectCommand* cmds = m_IndirectCommandBuffer.GetHeadContents() + m_NextIndirectCmdsCount;
    glm::vec4* paddedPos = m_PositionSSBO.GetHeadContents() + m_NextIndirectCmdsCount;

    // optimization can be done here if we can guarantee object buffer ranges wont be defragmented
    // only having one range for each object is better bcuz we would only need to have one indirect cmd per object
    // but that would require completely changing the archtecture of the GPUcache object lol
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

template <typename ChunkType>
void VoxelRenderer<ChunkType>::NextFrame()
{
    //if (m_NextIndirectCmdsCount == 0) return;

    m_ObjectsRenderedInNextFrame.swap(m_ObjectsRenderedInCurrentFrame);

    m_IndirectCommandBuffer.AdvanceHead();
    m_PositionSSBO.AdvanceHead();

    m_IndirectCommandBuffer.AdvanceTail();
    m_PositionSSBO.AdvanceTail();

    m_CurrentIndirectCmdsCount = m_NextIndirectCmdsCount;
    m_NextIndirectCmdsCount = 0;
}

template <typename ChunkType>
void VoxelRenderer<ChunkType>::ToggleDrawLines()
{
    m_DrawLines = !m_DrawLines;
}

template <typename ChunkType>
void VoxelRenderer<ChunkType>::Render(const Camera& camera)
{
    m_VoxelShaders.Use();
    m_VoxelShaders.SetMat4("view", camera.GetViewMatrix());
    m_VoxelShaders.SetMat4("projection", camera.GetProjMatrix());

    glBindVertexArray(m_VAO);
    m_PositionSSBO.BindTailBufferRange(m_CurrentIndirectCmdsCount);

    //assert(glGetError() == GL_NO_ERROR);


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

template <typename ChunkType>
void VoxelRenderer<ChunkType>::_RefreshFrame()
{
    std::cerr << "_RefreshFrame Not Implemented yet.\n";
}
#endif
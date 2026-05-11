#ifndef VOXEL_RENDERER_TPP
#define VOXEL_RENDERER_TPP
#include "voxel_renderer.h"

#include "frame_counter.h"

template <typename ChunkType>
VoxelRenderer<ChunkType>::VoxelRenderer(std::unique_ptr<VoxelMesher<ChunkType>> mesher)
    : m_DataCache(true)
    , m_IndirectCommandBuffer(true)
    , m_PositionSSBO(true)
    , m_UncachedChunks(true)
    , m_Mesher(std::move(mesher))
{
    PROFILE_FUNCTION();
}

template <typename ChunkType>
VoxelRenderer<ChunkType>::~VoxelRenderer() 
{
    PROFILE_FUNCTION();
    LOG_INFO(EngineSystem::RENDERER, "Destroying Voxel Renderer");

    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_QuadVBO);
}

template <typename ChunkType>
void VoxelRenderer<ChunkType>::Init(size_t cachePages, size_t cachePageSize, size_t indirectBufferSize)
{
    PROFILE_FUNCTION();
    LOG_INFO(EngineSystem::RENDERER,
        "Initializing VoxelRenderer (cachePages={}, cachePageSize={}, indirectBufferSize={})",
        cachePages, cachePageSize, indirectBufferSize);
    assert(cachePages * cachePageSize * 5 > indirectBufferSize && "Cache Size too small");
    m_MaxIndirectCommands = indirectBufferSize;
    _CompileShaders();
    _CreateGPUBuffers(indirectBufferSize, cachePageSize, cachePages);
    _EnableOpenGLFeatures();

    _SetupOpenGLAttribs();

    LOG_INFO(EngineSystem::RENDERER, "VoxelRenderer initialization complete");
}

template<typename ChunkType>
template <ChunkProvider<ChunkType> ChunkContainer>
void VoxelRenderer<ChunkType>::Upload(const ChunkContainer& chunkContainer, const glm::ivec3& chunkCoords)
{
    PROFILE_FUNCTION();

    VoxelObjectID chunkUID = UIDManager::Generate(chunkCoords);
    if (IsCached(chunkUID))
    {
        LOG_DEBUG(EngineSystem::RENDERER,
            "[Frame: {}] ChunkMeshUpdate chunk VoxelObjectHandle={} coords=({},{},{}) container VoxelObjectHandle={}",
            FrameCounter::Get(), chunkUID,
            chunkCoords.x, chunkCoords.y, chunkCoords.z,
            chunkContainer.GetUID());

        VoxelObjectID tempUID = UIDManager::Generate();
        std::vector<VoxelQuad> chunkMesh = m_Mesher->MeshChunk(chunkContainer, chunkCoords);
        m_DataCache.AllocateObject(tempUID, chunkMesh.data(), chunkMesh.size());

        m_DataCache.Swap(tempUID, chunkUID);
        //_RefreshFrame();
        m_DataCache.DeallocateObject(tempUID);
    }
    else
    {
        LOG_DEBUG(EngineSystem::RENDERER,
            "[Frame: {}] ChunkMeshNew chunk VoxelObjectHandle={} coords=({},{},{}) container VoxelObjectHandle={}",
            FrameCounter::Get(), chunkUID,
            chunkCoords.x, chunkCoords.y, chunkCoords.z,
            chunkContainer.GetUID());

        std::vector<VoxelQuad> chunkMesh = m_Mesher->MeshChunk(chunkContainer, chunkCoords);
        m_DataCache.AllocateObject(chunkUID, chunkMesh.data(), chunkMesh.size());
    }
}

template<typename ChunkType>
template <ChunkProvider<ChunkType> ChunkContainer>
void VoxelRenderer<ChunkType>::Upload(const ChunkContainer& chunkContainer)
{
    PROFILE_FUNCTION();

    VoxelObjectID containerUID = chunkContainer.GetUID();

    if (IsCached(containerUID))
    {
        LOG_DEBUG(EngineSystem::RENDERER,
            "[Frame: {}] ChunkContainerMeshUpdate VoxelObjectHandle={}",
            FrameCounter::Get(), containerUID);
        VoxelObjectID tempUID = UIDManager::Generate();
        std::vector<VoxelQuad> chunkMesh = m_Mesher->MeshChunk(chunkContainer);
        m_DataCache.AllocateObject(tempUID, chunkMesh.data(), chunkMesh.size());

        m_DataCache.Swap(tempUID, containerUID);
        //_RefreshFrame();
        m_DataCache.DeallocateObject(tempUID);
    }
    else
    {
        LOG_DEBUG(EngineSystem::RENDERER,
            "[Frame: {}] ChunkContainerMeshNew VoxelObjectHandle={}",
            FrameCounter::Get(), containerUID);

        std::vector<VoxelQuad> chunkMesh = m_Mesher->MeshChunkGrid(chunkContainer);
        m_DataCache.AllocateObject(containerUID, chunkMesh.data(), chunkMesh.size());
    }

}

template <typename ChunkType>
void VoxelRenderer<ChunkType>::DrawOnNextFrame(VoxelObjectID objectID, const glm::vec3& position)
{
    PROFILE_FUNCTION();

    if (!m_DataCache.Has(objectID))
    {
        LOG_WARN(EngineSystem::RENDERER, "[Frame: {}] Attempted to draw uncached voxel object (VoxelObjectHandle={})", 
            FrameCounter::Get(), objectID);
        return;
    }
    
    std::vector<GPUBufferRange> memoryRanges = m_DataCache.GetObjectBufferRanges(objectID);
    uint32_t* indirectCmdsCount = reinterpret_cast<uint32_t*>(m_IndirectCommandBuffer.GetHeadContents());

    const size_t headerSize = sizeof(uint32_t);
    const size_t bodyCount = (*indirectCmdsCount) * sizeof(DrawArraysIndirectCommand);
    DrawArraysIndirectCommand* cmds = reinterpret_cast<DrawArraysIndirectCommand*>(m_IndirectCommandBuffer.GetHeadContents() + headerSize + bodyCount);
    glm::vec4* paddedPos = m_PositionSSBO.GetHeadContents() + (*indirectCmdsCount);

    LOG_TRACE(EngineSystem::RENDERER,
        "[Frame: {}] Scheduling UID={} for draw (indirectCmds={})",
        FrameCounter::Get(), objectID, memoryRanges.size());

    // optimization can be done here if we can guarantee object buffer ranges wont be defragmented
    // only having one range for each object is better bcuz we would only need to have one indirect cmd per object
    // but that would require completely changing the archtecture of the GPUcache object lol
    for (const auto& range : memoryRanges)
    {
        cmds->count = 4;
        cmds->first = 0;
        cmds->baseInstance =  static_cast<unsigned int>(range.startOffset);
        cmds->instanceCount = static_cast<unsigned int>(range.length);

        paddedPos->x = position.x;
        paddedPos->y = position.y;
        paddedPos->z = position.z;
        paddedPos->w = 0;

        cmds++;
        paddedPos++;
    }

    std::atomic<uint32_t>* atomicCount = reinterpret_cast<std::atomic<uint32_t>*>(indirectCmdsCount);
    atomicCount->fetch_add(static_cast<uint32_t>(memoryRanges.size()), std::memory_order_relaxed);
}

template<typename ChunkType>
void VoxelRenderer<ChunkType>::DispatchFrustumCullPass(unsigned int renderDistance, const Camera& camera)
{
    PROFILE_FUNCTION();

    const Frustum& camFrustum = camera.GetFrustum();

    m_FrustumCullingShader.Use();
    
    m_FrustumCullingShader.SetVec4Array("u_FrustumPlanes", reinterpret_cast<const glm::vec4*>(&camFrustum), 6);
    m_FrustumCullingShader.SetUInt("u_RenderDistance", static_cast<unsigned int>(renderDistance / 2));
    m_FrustumCullingShader.SetUInt("u_ChunkSize", static_cast<unsigned int>(ChunkType::Size));
    m_FrustumCullingShader.SetIVec3("u_CameraChunkPos", WorldToChunk(glm::ivec3(camera.pos), static_cast<unsigned int>(ChunkType::Size)));

    std::byte* uncachedBase = m_UncachedChunks.GetCurrentContents();
    std::byte* indirectBase = m_IndirectCommandBuffer.GetCurrentContents();

    *reinterpret_cast<uint32_t*>(uncachedBase) = 0;
    *reinterpret_cast<uint32_t*>(indirectBase) = 0;

    GLsync clearCounts = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glWaitSync(clearCounts, 0, GL_TIMEOUT_IGNORED);
    glDeleteSync(clearCounts);

    const GLint indirectCmdsLocation = 0;
    const GLint chunkPosLocation = 1;
    const GLint uncachedChunksLocation = 2;
    const GLint hashMapLocation = 3;
    const GLint pageNodesBufferLocation = 4;
    const GLint policyBufferLocation = 5;

    m_IndirectCommandBuffer.BindCurrentFrame(indirectCmdsLocation);
    m_PositionSSBO.BindCurrentFrame(chunkPosLocation);
    m_UncachedChunks.BindCurrentFrame(uncachedChunksLocation);
    m_DataCache.BindCacheLookup(hashMapLocation, pageNodesBufferLocation, policyBufferLocation);

    glm::ivec3 localSize = m_FrustumCullingShader.GetLocalSizeGroup();
    unsigned int dimension = renderDistance * 2 + 1;
    unsigned int groupX = (dimension + localSize.x - 1) / localSize.x;
    unsigned int groupY = (dimension + localSize.y - 1) / localSize.y;
    unsigned int groupZ = (dimension + localSize.z - 1) / localSize.z;

    m_FrustumCullingShader.Dispatch(groupX, groupY, groupZ);
    m_FrustumCullingShader.Wait(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

    LOG_DEBUG(EngineSystem::RENDERER,
        "[Frame: {}] VoxelRenderer Dispatched Frustum Cull Pass",
        FrameCounter::Get());
}

template<typename ChunkType>
std::span<const glm::ivec4> VoxelRenderer<ChunkType>::GetGPURequestedChunks()
{
    PROFILE_FUNCTION();

    const std::byte* uncachedBase = m_UncachedChunks.GetPreviousContents();
    const uint32_t count = *reinterpret_cast<const uint32_t*>(uncachedBase);
    const glm::ivec4* results = reinterpret_cast<const glm::ivec4*>(uncachedBase + sizeof(uint32_t));

    {
        // NOTE: these calculations will get optimized out by compiler in O2/-O3 or /O2 
        const size_t headerSize = sizeof(uint32_t);
        const size_t requiredBytes = static_cast<size_t>(count) * sizeof(glm::ivec4);
        const size_t availableBytes = m_UncachedChunks.GetSize() - headerSize;

        assert(requiredBytes <= availableBytes &&
            "Overflow: Frustum culling SSBO size is too small for the result chunk count. "
            "Required bytes: {}, Available bytes: {}",
            requiredBytes, availableBytes);
    }

    LOG_DEBUG(EngineSystem::RENDERER,
        "[Frame: {}] GPU requested {} chunks to be meshed",
        FrameCounter::Get(),
        count);

    return std::span<const glm::ivec4>(results, count);
}

template <typename ChunkType>
void VoxelRenderer<ChunkType>::NextFrame()
{
    PROFILE_FUNCTION();

    LOG_TRACE(EngineSystem::RENDERER, "VoxelRenderer Advancing frame");

    m_IndirectCommandBuffer.Commit();
    m_PositionSSBO.Commit();
    m_UncachedChunks.Commit();
}

template <typename ChunkType>
void VoxelRenderer<ChunkType>::Render(const Camera& camera)
{
    PROFILE_FUNCTION();

    //const uint32_t indirectCmdsCount = *reinterpret_cast<const uint32_t*>(m_IndirectCommandBuffer.GetPreviousContents());

    //LOG_TRACE(EngineSystem::RENDERER,
    //    "VoxelRenderer rendering frame with {} indirect commands",
    //    indirectCmdsCount);
    
    //if (indirectCmdsCount == 0) return;

    m_VoxelShader.Use();
    m_VoxelShader.SetMat4("view", camera.GetViewMatrix());
    m_VoxelShader.SetMat4("projection", camera.GetProjMatrix());

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_IndirectCommandBuffer.GetName());
    glBindBuffer(GL_PARAMETER_BUFFER, m_IndirectCommandBuffer.GetName());

    const GLuint positionSSBOLocation = 0;
    m_PositionSSBO.BindPreviousFrame(positionSSBOLocation);

    //assert(glGetError() == GL_NO_ERROR);
    const size_t countOffset = m_IndirectCommandBuffer.GetPreviousFrameByteOffset();
    const size_t offset = sizeof(uint32_t) + countOffset;
    glMultiDrawArraysIndirectCount(GL_TRIANGLE_STRIP, reinterpret_cast<const void*>(offset), countOffset, m_MaxIndirectCommands, 0);
    //assert(glGetError() == GL_NO_ERROR);

    glBindVertexArray(0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindBuffer(GL_PARAMETER_BUFFER, 0);
}

template <typename ChunkType>
void VoxelRenderer<ChunkType>::_RefreshFrame()
{
    std::cerr << "_RefreshFrame Not Implemented yet.\n";
}

template<typename ChunkType>
void VoxelRenderer<ChunkType>::_CreateGPUBuffers(size_t indirectBufferSize, size_t cachePageSize, size_t cachePages)
{
    PROFILE_FUNCTION();

    m_DataCache.Create(GL_ARRAY_BUFFER, cachePageSize, cachePages);
    m_PositionSSBO.Create(GL_SHADER_STORAGE_BUFFER, indirectBufferSize);

    const size_t indirectHeaderSize = sizeof(uint32_t);
    const size_t indirectBodySize = indirectBufferSize * sizeof(DrawArraysIndirectCommand);
    const size_t indirectSSBOSize = indirectHeaderSize + indirectBodySize;
    m_IndirectCommandBuffer.Create(GL_SHADER_STORAGE_BUFFER, indirectSSBOSize, BufferAccess::ReadWrite);

    const size_t culledHeaderSize = sizeof(uint32_t);
    const size_t culledCoordsSize = sizeof(glm::vec4) * indirectBufferSize * 5; // TEMP: sizing, later will give correct sizing for frustum culling SSBO
    const size_t culledSSBOSize = culledHeaderSize + culledCoordsSize;
    m_UncachedChunks.Create(GL_SHADER_STORAGE_BUFFER, culledSSBOSize, BufferAccess::ReadWrite);

    // Offset head from tail on OrphanBuffers
    //m_IndirectCommandBuffer.AdvanceHead();
    //m_PositionSSBO.AdvanceHead();
    //m_UncachedChunks.AdvanceHead();

    LOG_DEBUG(EngineSystem::RENDERER, "GPU Buffers Created")
}

template<typename ChunkType>
void VoxelRenderer<ChunkType>::_CompileShaders()
{
    m_VoxelShader = Shader({ { "voxel_shader.vert.glsl", GL_VERTEX_SHADER },{ "voxel_shader.frag.glsl", GL_FRAGMENT_SHADER } });
    m_FrustumCullingShader = ComputeShader("terrian_culling_shader.comp.glsl");
}
template<typename ChunkType>
void VoxelRenderer<ChunkType>::_EnableOpenGLFeatures()
{
    glEnable(GL_DEPTH_TEST);
}

template<typename ChunkType>
void VoxelRenderer<ChunkType>::_SetupOpenGLAttribs()
{
    PROFILE_FUNCTION();

    // setup default quad that will be instanced
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_QuadVBO);

    LOG_DEBUG(EngineSystem::RENDERER, "VoxelRenderer generating quad VAO (ID={}) & VBO (ID={}) buffers.", m_VAO, m_QuadVBO);

    glBindVertexArray(m_VAO);
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
#endif
#ifndef VOXEL_RENDERER_TPP
#define VOXEL_RENDERER_TPP
#include "voxel_renderer.h"

template <typename ChunkType>
VoxelRenderer<ChunkType>::VoxelRenderer(std::unique_ptr<VoxelMesher<ChunkType>> mesher)
    : m_DataCache(true)
    , m_IndirectCommandBuffer(true)
    , m_PositionSSBO(true)
    , m_CulledChunkCoordsReadbackBuffer(true)
    , m_DrawLines(false)
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

    _CompileShaders();
    _CreateGPUBuffers(indirectBufferSize, cachePageSize, cachePages);
    _EnableOpenGLFeatures();

    m_ObjectsRenderedInCurrentFrame.reserve(indirectBufferSize);
    m_ObjectsRenderedInNextFrame.reserve(indirectBufferSize);

    _SetupOpenGLAttribs();

    LOG_INFO(EngineSystem::RENDERER, "VoxelRenderer initialization complete");
}

template<typename ChunkType>
template <ChunkProvider<ChunkType> ChunkContainer>
void VoxelRenderer<ChunkType>::Upload(const ChunkContainer& chunkContainer, const glm::ivec3& chunkCoords)
{
    PROFILE_FUNCTION();

    std::shared_ptr<const ChunkType> chunk = chunkContainer.GetChunk(chunkCoords);
    VoxelObjectID chunkUID = chunk->GetUID();
    GPUVoxelMeshCacheWriter meshWriter(m_DataCache);

    if (IsCached(chunkUID))
    {
        LOG_DEBUG(EngineSystem::RENDERER,
            "ChunkMeshUpdate chunk VoxelObjectHandle={} coords=({},{},{}) container VoxelObjectHandle={}",
            chunkUID,
            chunkCoords.x, chunkCoords.y, chunkCoords.z,
            chunkContainer.GetUID());

        VoxelObjectID tempUID = UIDManager::Generate();
        meshWriter.SetTargetObject(tempUID);
        m_Mesher->MeshChunk(chunkContainer, chunkCoords, meshWriter);

        m_DataCache.Swap(tempUID, chunkUID);
        //_RefreshFrame();
        m_DataCache.DeallocateObject(tempUID);
    }
    else
    {
        LOG_DEBUG(EngineSystem::RENDERER,
            "ChunkMeshNew chunk VoxelObjectHandle={} coords=({},{},{}) container VoxelObjectHandle={}",
            chunkUID,
            chunkCoords.x, chunkCoords.y, chunkCoords.z,
            chunkContainer.GetUID());

        meshWriter.SetTargetObject(chunkUID);
        m_Mesher->MeshChunk(chunkContainer, chunkCoords, meshWriter);
    }
}

template<typename ChunkType>
template <ChunkProvider<ChunkType> ChunkContainer>
void VoxelRenderer<ChunkType>::Upload(const ChunkContainer& chunkContainer)
{
    PROFILE_FUNCTION();

    VoxelObjectID containerUID = chunkContainer.GetUID();
    GPUVoxelMeshCacheWriter meshWriter(m_DataCache);

    if (IsCached(containerUID))
    {
        LOG_DEBUG(EngineSystem::RENDERER,
            "ChunkContainerMeshUpdate VoxelObjectHandle={}",
            containerUID);
        VoxelObjectID tempUID = UIDManager::Generate();
        meshWriter.SetTargetObject(tempUID);
        m_Mesher->MeshChunk(chunkContainer, meshWriter);

        m_DataCache.Swap(tempUID, containerUID);
        //_RefreshFrame();
        m_DataCache.DeallocateObject(tempUID);
    }
    else
    {
        LOG_DEBUG(EngineSystem::RENDERER,
            "ChunkContainerMeshNew VoxelObjectHandle={}",
            containerUID);

        meshWriter.SetTargetObject(containerUID);
        m_Mesher->MeshChunkGrid(chunkContainer, meshWriter);
    }

}

template <typename ChunkType>
void VoxelRenderer<ChunkType>::DrawOnNextFrame(VoxelObjectID objectID, const glm::vec3& position)
{
    PROFILE_FUNCTION();

    if (!m_DataCache.Has(objectID))
    {
        LOG_WARN(EngineSystem::RENDERER, "Attempted to draw uncached voxel object (VoxelObjectHandle={})", objectID);
        return;
    }
    
    std::vector<GPUBufferRange> memoryRanges = m_DataCache.GetObjectBufferRanges(objectID);
    DrawArraysIndirectCommand* cmds = m_IndirectCommandBuffer.GetHeadContents() + m_NextIndirectCmdsCount;
    glm::vec4* paddedPos = m_PositionSSBO.GetHeadContents() + m_NextIndirectCmdsCount;

    LOG_TRACE(EngineSystem::RENDERER,
        "Scheduling UID={} for draw (indirectCmds={})",
        objectID, memoryRanges.size());

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

    m_NextIndirectCmdsCount += memoryRanges.size();
    m_ObjectsRenderedInNextFrame.push_back(objectID);

}

template<typename ChunkType>
void VoxelRenderer<ChunkType>::DispatchFrustumCullPass(unsigned int renderDistance, const Camera& camera)
{
    const Frustum& camFrustum = camera.GetFrustum();

    m_FrustumCullingShader.Use();
    
    m_FrustumCullingShader.SetVec4Array("u_FrustumPlanes", reinterpret_cast<const glm::vec4*>(&camFrustum), 6);
    m_FrustumCullingShader.SetUInt("u_ChunkSize", static_cast<unsigned int>(ChunkType::Size));
    m_FrustumCullingShader.SetIVec3("u_CameraPos", WorldToChunk(glm::ivec3(camera.pos), static_cast<unsigned int>(ChunkType::Size)));
    assert(glGetError() == GL_NO_ERROR);

    std::byte* rawDataPtr = m_CulledChunkCoordsReadbackBuffer.GetHeadContents();
    *reinterpret_cast<uint32_t*>(rawDataPtr) = 0;

    m_CulledChunkCoordsReadbackBuffer.BindHeadBuffer(1);

    m_FrustumCullingShader.Dispatch(ceil(renderDistance / 8), ceil(renderDistance / 8), ceil(renderDistance / 4));

    m_CulledChunkCoordsReadbackBuffer.AdvanceHead();
}

template<typename ChunkType>
std::span<const glm::ivec4> VoxelRenderer<ChunkType>::GetFrustumCulledChunkCoords()
{
    m_CulledChunkCoordsReadbackBuffer.AdvanceTail();
    std::byte* rawDataPtr = m_CulledChunkCoordsReadbackBuffer.GetTailContents();
    assert(rawDataPtr != nullptr);

    // Read count (first 4 bytes)
    const uint32_t count = *reinterpret_cast<const uint32_t*>(rawDataPtr);

    // Results start immediately after `uint count`
    const std::byte* resultsPtr = rawDataPtr + sizeof(uint32_t);

    // Interpret GPU ivec4[] as glm::vec4[]
    const glm::ivec4* results = reinterpret_cast<const glm::ivec4*>(resultsPtr);

    return std::span<const glm::ivec4>(results, count);
}

template <typename ChunkType>
void VoxelRenderer<ChunkType>::NextFrame()
{
    PROFILE_FUNCTION();

    LOG_TRACE(EngineSystem::RENDERER,
        "VoxelRenderer advancing frame (indirectCmds={})",
        m_NextIndirectCmdsCount);

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

    LOG_INFO(EngineSystem::RENDERER,
        "VoxelRenderer draw mode set to {}",
        m_DrawLines ? "WIREFRAME" : "FILL");
}

template <typename ChunkType>
void VoxelRenderer<ChunkType>::Render(const Camera& camera)
{
    PROFILE_FUNCTION();

    m_VoxelShader.Use();
    m_VoxelShader.SetMat4("view", camera.GetViewMatrix());
    m_VoxelShader.SetMat4("projection", camera.GetProjMatrix());

    glBindVertexArray(m_VAO);
    m_PositionSSBO.BindTailBufferRange(0, m_CurrentIndirectCmdsCount);

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

template<typename ChunkType>
void VoxelRenderer<ChunkType>::_CreateGPUBuffers(size_t indirectBufferSize, size_t cachePageSize, size_t cachePages)
{
    PROFILE_FUNCTION();

    m_DataCache.Create(GL_ARRAY_BUFFER, cachePageSize, cachePages);
    m_IndirectCommandBuffer.Create(GL_DRAW_INDIRECT_BUFFER, indirectBufferSize, k_TripleBuffer);
    m_PositionSSBO.Create(GL_SHADER_STORAGE_BUFFER, indirectBufferSize, k_TripleBuffer);

    const size_t CulledHeaderSize = sizeof(uint32_t);
    const size_t CulledCoordsSize = sizeof(glm::vec4) * indirectBufferSize;
    const size_t CulledSSBOSize = CulledHeaderSize + CulledCoordsSize;
    m_CulledChunkCoordsReadbackBuffer.Create(GL_SHADER_STORAGE_BUFFER, CulledSSBOSize, k_TripleBuffer, BufferAccess::ReadWrite);

    // Offset head from tail on OrphanBuffers
    m_IndirectCommandBuffer.AdvanceHead();
    m_PositionSSBO.AdvanceHead();
    m_CulledChunkCoordsReadbackBuffer.AdvanceHead();

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
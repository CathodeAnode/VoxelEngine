#ifndef VOXEL_RENDERER_H
#define VOXEL_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glfw/glfw3.h>

#include <memory>
#include <vector>
#include <algorithm>
#include <cassert>

#include "logger.h"
#include "profiler.h"

#include "chunk.h"
#include "types.h"
#include "gpu_buffer_allocator.h"
#include "gpu_cache_allocator.h"
#include "chunk_provider_concept.h"
#include "shader.h"
#include "camera.h"
#include "voxel_math.h"
#include "frame_counter.h"
#include "debug_ui.h"

#define OPRPAH_BUFFERS_SIZE 3

template<typename ChunkType> 
class VoxelMesher;

template<typename ChunkType>
class VoxelRenderer 
{
public:
	VoxelRenderer(std::unique_ptr<VoxelMesher<ChunkType>> mesher);
	~VoxelRenderer();

	void Init(size_t cachePages, size_t cachePageSize, size_t renderBufferSize);

	template <ChunkProvider<ChunkType> ChunkContainer>
	void Upload(const ChunkContainer& chunkContainer, const glm::ivec3& chunkCoords);
	template <ChunkProvider<ChunkType> ChunkContainer>
	void Upload(const ChunkContainer& chunkContainer);

	bool UpdatePosition(VoxelObjectID objectID, const glm::vec3& newPosition);

	// TODO: needs to be updated to accommodate new changes
	[[deprecated]] void DrawOnNextFrame(VoxelObjectID objectID, const glm::vec3& position);

	void DispatchFrustumCullPass(unsigned int renderDistance, const Camera& camera);
	std::span<const glm::ivec4> GetGPURequestedChunks();

	void NextFrame();
	void Render(const Camera& camera);

	inline bool IsCached(VoxelObjectID objectID) { return m_DataCache.Has(objectID); }

private:

	GPUPagedCache<VoxelObjectID, VoxelQuad, FIFOPolicy> m_DataCache;
	static constexpr float m_QuadVertices[20] = {
		// position             texture
		0.0f,  0.0f, 0.0f,    0.0f, 1.0f,    // Bottom left
		0.0f,  1.0f, 0.0f,    1.0f, 1.0f,    // Top Left
		1.0f,  0.0f, 0.0f,    0.0f, 0.0f, // Bottom Right
		1.0f,  1.0f, 0.0f,    1.0f, 0.0f  // Top Right
	};

	GPUOrphanBuffer<glm::vec4, OPRPAH_BUFFERS_SIZE> m_PositionSSBO;
	GPUOrphanBuffer<std::byte, OPRPAH_BUFFERS_SIZE> m_IndirectCommandBuffer;
	GPUOrphanBuffer<std::byte, OPRPAH_BUFFERS_SIZE> m_UncachedChunks; // TODO figure out sizing

	ComputeShader m_FrustumCullingShader;

	std::unique_ptr<VoxelMesher<ChunkType>> m_Mesher;
	GLsizei m_MaxIndirectCommands;

	Shader m_VoxelShader;

	unsigned int m_VAO, m_QuadVBO;

private:
	inline void _CreateGPUBuffers(size_t indirectBufferSize, size_t cachePageSize, size_t cachePages);
	inline void _CompileShaders();
	inline void _EnableOpenGLFeatures();
	inline void _SetupOpenGLAttribs();
};

typedef VoxelRenderer<Chunk8> VoxelRenderer8;
typedef VoxelRenderer<Chunk16> VoxelRenderer16;
typedef VoxelRenderer<Chunk32> VoxelRenderer32;

#include "voxel_renderer.tpp"

#endif
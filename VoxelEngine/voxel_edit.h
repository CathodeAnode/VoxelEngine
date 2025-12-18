#ifndef VOXEL_EDIT_API_H
#define VOXEL_EDIT_API_H

#include <glm/glm.hpp>

#include "types.h"
#include "chunk_provider_concept.h"


template<typename ChunkType>
class VoxelRenderer;

template <typename ChunkType, ChunkProvider<ChunkType> ChunkContainer>
class VoxelEditAPI
{
public:
	VoxelEditAPI(ChunkContainer& chunkContainer, VoxelRenderer<ChunkType>& renderer);

	void SetVoxel(const glm::ivec3& coords, RGBAColor color);
	void RemoveVoxel(const glm::ivec3& coords);

	void SetBoxVolume(const Point& A, const Point& B, RGBAColor color);
	void RemoveBoxVolume(const Point& A, const Point& B);

	void SetSphere(glm::ivec3 coords, uint16_t radius, RGBAColor color);
	void RemoveSphere(glm::ivec3 coords, uint16_t radius);

	void SwitchContext(Terrian<ChunkType>& context);

private:
	ChunkContainer& m_ChunkContainer;
	VoxelRenderer<ChunkType>& m_Renderer;


	inline static glm::ivec3 _VoxelToChunkPos(const glm::ivec3& voxelWorldPos);
	inline static glm::ivec3 _WorldToLocalPos(const glm::ivec3& voxelWorldPos);
};

#include "voxel_edit.tpp"

#endif



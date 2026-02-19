#ifndef VOXEL_EDIT_API_H
#define VOXEL_EDIT_API_H

#include <glm/glm.hpp>

#include "types.h"
#include "chunk_provider_concept.h"
#include "voxel_math.h"


template<typename ChunkType>
class VoxelRenderer;

template <typename ChunkType, ChunkProvider<ChunkType> ChunkContainer>
class VoxelEdit
{
public:
	VoxelEdit(ChunkContainer& chunkContainer, VoxelRenderer<ChunkType>& renderer);

	void SetVoxel(const glm::ivec3& coords, RGBAColor color);
	void RemoveVoxel(const glm::ivec3& coords);

	void SetBoxVolume(const glm::ivec3& A, const glm::ivec3& B, RGBAColor color);
	void RemoveBoxVolume(const glm::ivec3& A, const glm::ivec3& B);

	void SetSphere(glm::ivec3 coords, uint16_t radius, RGBAColor color);
	void RemoveSphere(glm::ivec3 coords, uint16_t radius);

	void SwitchContext(ChunkContainer& context);

private:
	ChunkContainer& m_ChunkContainer;
	VoxelRenderer<ChunkType>& m_Renderer;
};

#include "voxel_edit.tpp"

#endif



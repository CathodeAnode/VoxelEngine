#ifndef VOXEL_EDIT_API_TPP
#define VOXEL_EDIT_API_TPP

#include "voxel_edit.h"

template <typename ChunkType, ChunkProvider<ChunkType> ChunkContainer>
VoxelEdit<ChunkType, ChunkContainer>::VoxelEdit(ChunkContainer& chunkContainer, VoxelRenderer<ChunkType>& renderer)
	: m_ChunkContainer(chunkContainer)
	, m_Renderer(renderer)
{}

template <typename ChunkType, ChunkProvider<ChunkType> ChunkContainer>
void VoxelEdit<ChunkType, ChunkContainer>::SetVoxel(const glm::ivec3& coords, RGBAColor color)
{
	const glm::ivec3 chunkCoords = WorldToChunk(coords, ChunkType::Size);
	std::shared_ptr<const ChunkType> chunk = m_ChunkContainer.GetChunk(chunkCoords);

	assert(chunk->GetUID() != 0);

	const glm::ivec3 localVoxelCoords = VoxelToLocal(coords, ChunkType::Size);

	chunk->m_OpaqueData[ChunkType::_GetOpaqueDataIndex(localVoxelCoords.x, localVoxelCoords.z)] |= (1 << localVoxelCoords.y);
	chunk->m_ColorData[ChunkType::_GetColorDataIndex(localVoxelCoords.x, localVoxelCoords.y, localVoxelCoords.z)] = color;

	m_Renderer.Upload(m_ChunkContainer, chunkCoords);

	for (int axis = 0; axis < 3; axis++)
	{
		glm::ivec3 offset{ 0 };
		if (localVoxelCoords[axis] == 0)
		{
			offset[axis] = -1;
			m_Renderer.Upload(m_ChunkContainer, chunkCoords + offset);
		}
		else if (localVoxelCoords[axis] == ChunkType::Size - 1)
		{
			offset[axis] = 1;
			m_Renderer.Upload(m_ChunkContainer, chunkCoords + offset);
		}
	}
}

template<typename ChunkType, ChunkProvider<ChunkType> ChunkContainer>
void VoxelEdit<ChunkType, ChunkContainer>::RemoveVoxel(const glm::ivec3& coords)
{
	const glm::ivec3 chunkCoords = WorldToChunk(coords, ChunkType::Size);
	std::shared_ptr<const ChunkType> chunk = m_ChunkContainer.GetChunk(chunkCoords);

	assert(chunk->GetUID() != 0);

	const glm::ivec3 localVoxelCoords = VoxelToLocal(coords, ChunkType::Size);

	chunk->m_OpaqueData[ChunkType::_GetOpaqueDataIndex(localVoxelCoords.x, localVoxelCoords.z)] &= ~(1 << localVoxelCoords.y);

	m_Renderer.Upload(m_ChunkContainer, chunkCoords);

	for (int axis = 0; axis < 3; axis++)
	{
		glm::ivec3 offset{ 0 };
		if (localVoxelCoords[axis] == 0)
		{
			offset[axis] = -1;
			m_Renderer.Upload(m_ChunkContainer, chunkCoords + offset);
		}
		else if (localVoxelCoords[axis] == ChunkType::Size - 1)
		{
			offset[axis] = 1;
			m_Renderer.Upload(m_ChunkContainer, chunkCoords + offset);
		}
	}

}

#endif
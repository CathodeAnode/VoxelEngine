#ifndef VOXEL_EDIT_API_TPP
#define VOXEL_EDIT_API_TPP

#include "voxel_edit.h"

template <typename ChunkType, ChunkProvider<ChunkType> ChunkContainer>
VoxelEdit<ChunkType, ChunkContainer>::VoxelEdit(ChunkContainer& chunkContainer, unsigned int cap)
	: m_ChunkContainer(chunkContainer)
	, m_DirtyChunks(cap)
{
}

template<typename ChunkType, ChunkProvider<ChunkType> ChunkContainer>
ViewRingBuffer<glm::ivec3>::Range VoxelEdit<ChunkType, ChunkContainer>::GetDirtyChunks(unsigned int budget)
{
	ViewRingBuffer<glm::ivec3>::Range view = m_DirtyChunks.View(budget);
	m_DirtyChunks.Consume(view.Size());
	return view;
}

template <typename ChunkType, ChunkProvider<ChunkType> ChunkContainer>
void VoxelEdit<ChunkType, ChunkContainer>::SetVoxel(const glm::ivec3& coords, RGBAColor color)
{
	LOG_DEBUG(EngineSystem::VOXEL_MESHER
		, "Setting voxel at ({},{},{}) to color=0x{:08X}"
		, coords.x, coords.y, coords.z
		, color);

	const glm::ivec3 chunkCoords = WorldToChunk(coords, ChunkType::Size);
	std::shared_ptr<ChunkType> chunk = m_ChunkContainer.GetChunk(chunkCoords);

	assert(chunk->GetUID() != 0);

	const glm::ivec3 localVoxelCoords = VoxelToLocal(coords, ChunkType::Size);

	auto chunkOpaqueData = chunk->GetOpaqueSpan();
	auto chunkColorData = chunk->GetColorSpan();

	chunkOpaqueData[ChunkType::OpaqueDataIndexAt(localVoxelCoords.x, localVoxelCoords.z)] |= (1 << localVoxelCoords.y);
	chunkColorData[ChunkType::ColorDataIndexAt(localVoxelCoords.x, localVoxelCoords.y, localVoxelCoords.z)] = color;

	m_DirtyChunks.Push(chunkCoords);
}

template<typename ChunkType, ChunkProvider<ChunkType> ChunkContainer>
void VoxelEdit<ChunkType, ChunkContainer>::RemoveVoxel(const glm::ivec3& coords)
{
	LOG_DEBUG(EngineSystem::VOXEL_MESHER
		, "Removing voxel at ({},{},{})"
		, coords.x, coords.y, coords.z);


	const glm::ivec3 chunkCoords = WorldToChunk(coords, ChunkType::Size);
	std::shared_ptr<ChunkType> chunk = m_ChunkContainer.GetChunk(chunkCoords);

	assert(chunk->GetUID() != 0);

	const glm::ivec3 localVoxelCoords = VoxelToLocal(coords, ChunkType::Size);

	auto chunkOpaqueData = chunk->GetOpaqueSpan();
	chunkOpaqueData[ChunkType::OpaqueDataIndexAt(localVoxelCoords.x, localVoxelCoords.z)] &= ~(1 << localVoxelCoords.y);

	m_DirtyChunks.Push(chunkCoords);

}

#endif
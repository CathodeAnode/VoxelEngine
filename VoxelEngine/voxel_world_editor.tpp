#include "voxel_world_editor.h"

template<typename ChunkType>
VoxelWorldEditor<ChunkType>::VoxelWorldEditor(World<ChunkType>& world)
	: m_World(world)
{}

template<typename ChunkType>
void VoxelWorldEditor<ChunkType>::SetVoxel(const glm::ivec3& coords, RGBAColor color)
{
	ChunkType* chunk = m_World.GetChunk(_VoxelToChunkPos(coords));

	assert(chunk->GetUID() != NULL);

	glm::ivec3 localChunkCoords = _WorldToLocalPos(coords);

	chunk->m_OpaqueData[ChunkType::_GetOpaqueDataIndex(localChunkCoords.x, localChunkCoords.z)] |= (1 << localChunkCoords.y);
	chunk->m_ColorData[ChunkType::_GetColorDataIndex(localChunkCoords.x, localChunkCoords.y, localChunkCoords.z)] = color;
}

template<typename ChunkType>
inline glm::ivec3 VoxelWorldEditor<ChunkType>::_VoxelToChunkPos(const glm::ivec3& voxelWorldPos)
{
	return glm::ivec3(
		floor(voxelWorldPos.x / (float)ChunkType::Size),
		floor(voxelWorldPos.y / (float)ChunkType::Size),
		floor(voxelWorldPos.z / (float)ChunkType::Size)
	);
}

template<typename ChunkType>
inline glm::ivec3 VoxelWorldEditor<ChunkType>::_WorldToLocalPos(const glm::ivec3& voxelWorldPos)
{
	return glm::ivec3(
		((voxelWorldPos.x % ChunkType::Size) + ChunkType::Size) % ChunkType::Size,
		((voxelWorldPos.y % ChunkType::Size) + ChunkType::Size) % ChunkType::Size,
		((voxelWorldPos.z % ChunkType::Size) + ChunkType::Size) % ChunkType::Size
	);
}

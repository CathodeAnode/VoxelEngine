#include "voxel_world_editor.h"

template<typename ChunkType>
VoxelWorldEditor<ChunkType>::VoxelWorldEditor(World<ChunkType>& world)
	: m_World(world)
{}

template<typename ChunkType>
void VoxelWorldEditor<ChunkType>::SetVoxel(const glm::ivec3& coords, RGBAColor color)
{
	ChunkType* chunk = m_World.GetChunk(_VoxelToChunkPos(coords));

	if (chunk->GetUID() == NULL)
	{
		// create new chunk and allocate it to world
	}

	chunk->m_OpqueData[...] |= (1 << ...);
	chunk->m_ColorData[...] = color;
}

template<typename ChunkType>
inline glm::ivec3 VoxelWorldEditor<ChunkType>::_VoxelToChunkPos(const glm::ivec3& voxelWorldPos)
{
	return glm::ivec3(
		floor(voxelWorldPos.x / (float)chunkSize),
		floor(voxelWorldPos.y / (float)chunkSize),
		floor(voxelWorldPos.z / (float)chunkSize)
	);
}

template<typename ChunkType>
inline glm::ivec3 VoxelWorldEditor<ChunkType>::_WorldToLocalPos(const glm::ivec3& voxelWordlPos)
{
	return glm::ivec3(
		((voxelWorldPos.x % ChunkSize) + ChunkSize) % ChunkSize;
		((voxelWorldPos.y % ChunkSize) + ChunkSize) % ChunkSize;
		((voxelWorldPos.z % ChunkSize) + ChunkSize) % ChunkSize;
	);
}

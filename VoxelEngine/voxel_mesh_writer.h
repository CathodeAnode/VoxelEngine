#ifndef VOXEL_MESH_WRITER_H
#define VOXEL_MESH_WRITER_H

#include "types.h"
#include "gpu_buffer_allocator.h"

template<typename T>
concept VoxelMeshWriter = requires(T t, QuadMeshData data, Color type)
{
	t.Write(data, type);
};

class CPUVoxelMeshWriter
{
public:
	CPUVoxelMeshWriter(ChunkQuads& chunkQuads);

	void Write(QuadMeshData quadData, Color quadColor);

private:
	ChunkQuads& m_Container;
};

class GPUVoxelMeshCacheWriter
{
public:
	GPUVoxelMeshCacheWriter(GPUPagedLRUCache<QuadMeshData, VoxelObjectID>& cache);

	void Write(QuadMeshData quadData, Color quadColor);
	void SetTargetObject(VoxelObjectID obj);

private:
	GPUPagedLRUCache<QuadMeshData, VoxelObjectID>& m_Cache;
	VoxelObjectID m_TargetObjectID;
};

#endif


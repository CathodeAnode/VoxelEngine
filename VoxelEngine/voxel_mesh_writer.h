#ifndef VOXEL_MESH_WRITER_H
#define VOXEL_MESH_WRITER_H

#include "types.h"
#include "gpu_cache_allocator.h"

template<typename T>
concept VoxelMeshWriter = requires(T t, QuadMeshData data, RGBAColor type)
{
	t.Write(data, type);
};

class CPUVoxelMeshWriter
{
public:
	CPUVoxelMeshWriter(ChunkQuads& chunkQuads);

	void Write(QuadMeshData quadData, RGBAColor quadColor);

private:
	ChunkQuads& m_Container;
};

class GPUVoxelMeshCacheWriter
{
public:
	GPUVoxelMeshCacheWriter(GPUPagedLRUCache<VoxelObjectID, VoxelQuad>& cache);

	void Write(QuadMeshData quadData, RGBAColor quadColor);
	void SetTargetObject(VoxelObjectID obj);

private:
	GPUPagedLRUCache<VoxelObjectID, VoxelQuad>& m_Cache;
	VoxelObjectID m_TargetObjectID;
};

#endif


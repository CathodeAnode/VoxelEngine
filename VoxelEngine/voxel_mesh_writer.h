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

template<typename Cache>
class GPUVoxelMeshCacheWriter
{
public:
	GPUVoxelMeshCacheWriter(Cache& cache)
		: m_Cache(cache)
		, m_TargetObjectID(0)
	{}

	void Write(QuadMeshData quadData, RGBAColor quadColor)
	{
		m_Cache.PushBackToObject(m_TargetObjectID, { quadData, quadColor });
	}
	void SetTargetObject(VoxelObjectID obj)
	{
		m_TargetObjectID = obj;
	}

private:
	Cache& m_Cache;
	VoxelObjectID m_TargetObjectID;
};

#endif


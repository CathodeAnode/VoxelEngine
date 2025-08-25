#include "voxel_mesh_writer.h"

CPUVoxelMeshWriter::CPUVoxelMeshWriter(ChunkQuads& chunkQuads)
	: m_Container(chunkQuads)
{
	m_Container = chunkQuads;
}

void CPUVoxelMeshWriter::Write(QuadMeshData quadData, Color quadColor)
{
	m_Container.AddRawQuad(quadData, quadColor);
}

// ------------------------------------------------------------------------------------------------------------------

GPUVoxelMeshCacheWriter::GPUVoxelMeshCacheWriter(GPUPagedLRUCache<QuadMeshData, VoxelObjectID>& cache)
	: m_Cache(cache)
{
	m_Cache = cache;
}

void GPUVoxelMeshCacheWriter::Write(QuadMeshData quadData, Color quadColor)
{
	m_Cache.PushBackToObject(m_TargetObjectID, quadData);
	// TODO: pushback both quad color & data
}

void GPUVoxelMeshCacheWriter::SetTargetObject(VoxelObjectID obj)
{
	m_TargetObjectID = obj;
}

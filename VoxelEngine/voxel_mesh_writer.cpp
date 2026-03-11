#include "voxel_mesh_writer.h"

CPUVoxelMeshWriter::CPUVoxelMeshWriter(ChunkQuads& chunkQuads)
	: m_Container(chunkQuads)
{}

void CPUVoxelMeshWriter::Write(QuadMeshData quadData, RGBAColor quadColor)
{
	m_Container.AddRawQuad(quadData, quadColor);
}

// ------------------------------------------------------------------------------------------------------------------

GPUVoxelMeshCacheWriter::GPUVoxelMeshCacheWriter(GPUPagedLRUCache<VoxelObjectID, VoxelQuad>& cache)
	: m_Cache(cache)
	, m_TargetObjectID(0)
{}

void GPUVoxelMeshCacheWriter::Write(QuadMeshData quadData, RGBAColor quadColor)
{
	m_Cache.PushBackToObject(m_TargetObjectID, {quadData, quadColor});
}

void GPUVoxelMeshCacheWriter::SetTargetObject(VoxelObjectID obj)
{
	m_TargetObjectID = obj;
}

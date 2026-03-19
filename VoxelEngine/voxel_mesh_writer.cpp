#include "voxel_mesh_writer.h"

CPUVoxelMeshWriter::CPUVoxelMeshWriter(ChunkQuads& chunkQuads)
	: m_Container(chunkQuads)
{}

void CPUVoxelMeshWriter::Write(QuadMeshData quadData, RGBAColor quadColor)
{
	m_Container.AddRawQuad(quadData, quadColor);
}

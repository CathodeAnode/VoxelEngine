#ifndef VOXELMESHER_H
#define VOXELMESHER_H

#include <vector>
#include <iostream>
#include <unordered_map>
#include <array>

#include <glm/glm.hpp>

#include "chunk.h"
#include "chunk_provider_concept.h"
#include "voxel_mesh_writer.h"
#include "types.h"
#include "helpers.h"


template<typename ChunkType> 
class VoxelMesher
{
public:

	template<ChunkProvider<ChunkType> ChunkContainer, VoxelMeshWriter MeshWriter>
	void MeshChunk(const ChunkContainer& chunkGrid, const glm::ivec3& chunkLocation, MeshWriter& out);

	template<ChunkProvider<ChunkType> ChunkContianer, VoxelMeshWriter MeshWriter>
	void MeshChunkGrid(const ChunkContianer& chunkGrid, MeshWriter& out);

private:
	static constexpr int CS = ChunkType::Size;
	static constexpr int CS_P = CS + 2;
	static constexpr int CS_2 = CS * CS;
	static constexpr int CS_P2 = CS_P * CS_P;
	static constexpr int CS_P3 = CS_P2 * CS_P;

	using VoxelColumnData = typename ChunkType::ValueType;
	using FaceVisibilityMasks = std::array<VoxelColumnData, CS_2 * 6>;
	using ColorFaceMasksMap = std::unordered_map<RGBAColor, FaceVisibilityMasks>;

	FaceVisibilityMasks m_FaceMasks;

private:
	template<ChunkProvider<ChunkType> ChunkContainer>
	inline static VoxelColumnData _GetPaddedColumnRowBits(const ChunkContainer& world, int x, int z, const glm::ivec3& chunkLocation);

	inline static QuadMeshData _CompressQuadData(uint8_t x, uint8_t y, uint8_t z, uint8_t w, uint8_t h, uint8_t dir);
	inline ColorFaceMasksMap _SplitVoxelsByColor(uint8_t axis, std::shared_ptr<const ChunkType> chunk);
};


typedef VoxelMesher<Chunk8> VoxelMesher8;
typedef VoxelMesher<Chunk16> VoxelMesher16;
typedef VoxelMesher<Chunk32> VoxelMesher32;

#include "voxel_mesher.tpp"

#endif



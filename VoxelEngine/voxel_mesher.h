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
#include "logger.h"
#include "profiler.h"

template<typename ChunkType>
struct ChunkData
{
	// Center chunk (current chunk being meshed)
	std::shared_ptr<const ChunkType> main;

	// X axis (left / right)
	std::shared_ptr<const ChunkType> xPos; // right neighbor
	std::shared_ptr<const ChunkType> xNeg; // left neighbor

	// Z axis (forward / backward)
	std::shared_ptr<const ChunkType> zPos; // forward neighbor
	std::shared_ptr<const ChunkType> zNeg; // backward neighbor

	// Y axis (up / down)
	std::shared_ptr<const ChunkType> yPos; // top neighbor
	std::shared_ptr<const ChunkType> yNeg; // bottom neighbor

	std::array<std::shared_ptr<const ChunkType>, 9> neighbors;

	template<ChunkProvider<ChunkType> ChunkContainer>
	void GatherData(const ChunkContainer& chunkContainer, const glm::ivec3& chunkLocation)
	{
		PROFILE_FUNCTION();

		// Center
		main = chunkContainer.GetChunk(chunkLocation);

		// X neighbors
		xPos = chunkContainer.GetChunk(chunkLocation + glm::ivec3(1, 0, 0));  // right
		xNeg = chunkContainer.GetChunk(chunkLocation + glm::ivec3(-1, 0, 0)); // left

		// Z neighbors
		zPos = chunkContainer.GetChunk(chunkLocation + glm::ivec3(0, 0, 1));  // forward
		zNeg = chunkContainer.GetChunk(chunkLocation + glm::ivec3(0, 0, -1)); // backward

		// Y neighbors
		yPos = chunkContainer.GetChunk(chunkLocation + glm::ivec3(0, 1, 0));  // above
		yNeg = chunkContainer.GetChunk(chunkLocation + glm::ivec3(0, -1, 0)); // below

		neighbors = {
			nullptr, zNeg,   nullptr,
			xNeg,    main,   xPos,
			nullptr, zPos,   nullptr
		};
	}

	typename VoxelMesher<ChunkType>::VoxelColumnData GetPaddedColumnRowBits(int x, int z) const
	{
		PROFILE_FUNCTION();

		const int offX = (x == 0) * -1 + (x == VoxelMesher<ChunkType>::CS_P - 1) * 1;
		const int offZ = (z == 0) * -1 + (z == VoxelMesher<ChunkType>::CS_P - 1) * 1;

		const int localX = (x + VoxelMesher<ChunkType>::CS - 1) % VoxelMesher<ChunkType>::CS;
		const int localZ = (z + VoxelMesher<ChunkType>::CS - 1) % VoxelMesher<ChunkType>::CS;

		const int index = (offZ + 1) * 3 + (offX + 1);
		const std::shared_ptr<const ChunkType>& chunk = neighbors[index];

		return chunk ? chunk->GetColumnRow(localX, localZ) : 0;
	}
};


template<typename ChunkType> 
class VoxelMesher
{
public:
	VoxelMesher();

	template<ChunkProvider<ChunkType> ChunkContainer, VoxelMeshWriter MeshWriter>
	void MeshChunk(const ChunkContainer& chunkContainer, const glm::ivec3& chunkLocation, MeshWriter& out);

	template<ChunkProvider<ChunkType> ChunkContianer, VoxelMeshWriter MeshWriter>
	void MeshChunkGrid(const ChunkContianer& chunkContainer, MeshWriter& out);

private:
	static constexpr int CS = ChunkType::Size;
	static constexpr int CS_P = CS + 2;
	static constexpr int CS_2 = CS * CS;
	static constexpr int CS_P2 = CS_P * CS_P;
	static constexpr int CS_P3 = CS_P2 * CS_P;

	friend struct ChunkData<ChunkType>;

private:
	using VoxelColumnData = typename ChunkType::ValueType;
	using FaceVisibilityMasks = std::array<VoxelColumnData, CS_2 * 6>;
	using ColorFaceMasksMap = std::unordered_map<RGBAColor, FaceVisibilityMasks>;

private:
	FaceVisibilityMasks m_FaceMasks;

private:
	inline static QuadMeshData _CompressQuadData(uint8_t x, uint8_t y, uint8_t z, uint8_t w, uint8_t h, uint8_t dir);
	inline ColorFaceMasksMap _SplitVoxelsByColor(uint8_t axis, std::shared_ptr<const ChunkType> chunk);
};


typedef VoxelMesher<Chunk8> VoxelMesher8;
typedef VoxelMesher<Chunk16> VoxelMesher16;
typedef VoxelMesher<Chunk32> VoxelMesher32;

#include "voxel_mesher.tpp"

#endif



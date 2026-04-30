#ifndef VOXELMESHER_H
#define VOXELMESHER_H

#include <vector>
#include <iostream>
#include <unordered_map>
#include <array>

#include <glm/glm.hpp>

#include "chunk.h"
#include "chunk_provider_concept.h"
#include "types.h"
#include "helpers.h"
#include "logger.h"
#include "profiler.h"

template<typename ChunkType>
struct ChunkData
{
	static const size_t DATA_SIZE = VoxelMesher<ChunkType>::CS_P * VoxelMesher<ChunkType>::CS_P;
	using T = typename ChunkType::ValueType;

	std::array<T, DATA_SIZE> paddedData;
	std::shared_ptr<const ChunkType> center;
	std::shared_ptr<const ChunkType> yPos;
	std::shared_ptr<const ChunkType> yNeg;
	

	template<ChunkProvider<ChunkType> ChunkContainer>
	void GatherData(const ChunkContainer& chunkContainer, const glm::ivec3& chunkLocation)
	{
		PROFILE_FUNCTION();

		// Center
		center = chunkContainer.GetChunk(chunkLocation);

		// X neighbors
		std::shared_ptr<const ChunkType> xPos = chunkContainer.GetChunk(chunkLocation + glm::ivec3(1, 0, 0));  // right
		std::shared_ptr<const ChunkType> xNeg = chunkContainer.GetChunk(chunkLocation + glm::ivec3(-1, 0, 0)); // left

		// Z neighbors
		std::shared_ptr<const ChunkType> zPos = chunkContainer.GetChunk(chunkLocation + glm::ivec3(0, 0, 1));  // forward
		std::shared_ptr<const ChunkType> zNeg = chunkContainer.GetChunk(chunkLocation + glm::ivec3(0, 0, -1)); // backward

		// Y neighbors
		yPos = chunkContainer.GetChunk(chunkLocation + glm::ivec3(0, 1, 0));  // above
		yNeg = chunkContainer.GetChunk(chunkLocation + glm::ivec3(0, -1, 0)); // below

		// copy center chunk opaque data
		auto* data = center->GetOpaqueData();
		for (int i = 0; i < VoxelMesher<ChunkType>::CS; ++i)
		{
			std::memcpy(&paddedData[1 + ((i+1) * VoxelMesher<ChunkType>::CS_P)],
				&data[0 + (i * VoxelMesher<ChunkType>::CS)],
				VoxelMesher<ChunkType>::CS);
		}

		// copy neighbour z-chunks opaque data
		// z postive
		data = zPos->GetOpaqueData();
		std::memcpy(&paddedData[1],
			&data[0 + (VoxelMesher<ChunkType>::CS - 1) * VoxelMesher<ChunkType>::CS],
			VoxelMesher<ChunkType>::CS);

		// z negative
		data = zNeg->GetOpaqueData();
		std::memcpy(&paddedData[1 + (VoxelMesher<ChunkType>::CS_P - 1) * VoxelMesher<ChunkType>::CS_P],
			&data[0],
			VoxelMesher<ChunkType>::CS);

		// x postive
		data = xPos->GetOpaqueData();
		for (int i = 0; i < VoxelMesher<ChunkType>::CS; ++i)
		{
			paddedData[0 + ((i + 1) * VoxelMesher<ChunkType>::CS_P)]
				= data[(VoxelMesher<ChunkType>::CS - 1) + i * VoxelMesher<ChunkType>::CS];
		}

		// x negitive
		data = xNeg->GetOpaqueData();
		for (int i = 0; i < VoxelMesher<ChunkType>::CS; ++i)
		{
			paddedData[(VoxelMesher<ChunkType>::CS_P - 1) + (i + 1) * VoxelMesher<ChunkType>::CS_P]
				= data[0 + i * VoxelMesher<ChunkType>::CS];
		}
	}

	typename VoxelMesher<ChunkType>::VoxelColumnData GetPaddedColumnRowBits(int x, int z) const
	{
		// Reject completely invalid or corner out-of-bounds accesses
		assert((x > 0 && z > 0) || x >= 0 || z >= 0 ||
			(x < VoxelMesher<ChunkType>::CS_P && z < VoxelMesher<ChunkType>::CS_P) ||
			(x != 0 && z != VoxelMesher<ChunkType>::CS_P - 1) ||
			(x != VoxelMesher<ChunkType>::CS_P - 1 && z != 0), "Invalid coordinates");


		return paddedData[x + z * VoxelMesher<ChunkType>::CS_P];
	}
};


template<typename ChunkType> 
class VoxelMesher
{
public:
	VoxelMesher();

	template<ChunkProvider<ChunkType> ChunkContainer>
	[[nodiscard]] std::vector<VoxelQuad> MeshChunk(const ChunkContainer& chunkContainer, const glm::ivec3& chunkLocation);

	template<ChunkProvider<ChunkType> ChunkContianer>
	[[nodiscard]] std::vector<VoxelQuad> MeshChunkGrid(const ChunkContianer& chunkContainer);

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
	FaceVisibilityMasks m_FaceMasks;    // TODO: make thread_local
	std::vector<VoxelQuad> m_ChunkMesh; // TODO: make thread_local

private:
	inline static QuadMeshData _CompressQuadData(uint8_t x, uint8_t y, uint8_t z, uint8_t w, uint8_t h, uint8_t dir);
	inline ColorFaceMasksMap _SplitVoxelsByColor(uint8_t axis, std::shared_ptr<const ChunkType> chunk);
};


typedef VoxelMesher<Chunk8> VoxelMesher8;
typedef VoxelMesher<Chunk16> VoxelMesher16;
typedef VoxelMesher<Chunk32> VoxelMesher32;

#include "voxel_mesher.tpp"

#endif



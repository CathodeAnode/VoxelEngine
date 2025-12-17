#ifndef VOXELMESHER_H
#define VOXELMESHER_H

#include <vector>
#include <iostream>
#include <unordered_map>
#include <stdexcept>

#include <glm/glm.hpp>

#include <bitset>
#include <array>

#include "chunk.h"
#include "chunk_grid.h"
#include "chunk_provider_concept.h"
#include "voxel_mesh_writer.h"
#include "types.h"
#include "helpers.h"


template<typename ChunkType> 
class VoxelMesher
{
private:
	using uintC_t = typename ChunkType::ValueType; // TODO change to a more meaningful name

	static constexpr int CS = ChunkType::Size;
	static constexpr int CS_P = CS + 2;
	static constexpr int CS_2 = CS * CS;
	static constexpr int CS_P2 = CS_P * CS_P;
	static constexpr int CS_P3 = CS_P2 * CS_P;

	std::array<uintC_t, CS_2 * 6> m_FaceMasks;

	template<ChunkProvider<ChunkType> ChunkContainer>
	static uintC_t _GetPaddedColumnRowBits(const ChunkContainer& world, int x, int z, const glm::ivec3& chunkLocation) {
		// Reject completely invalid or corner out-of-bounds accesses
		assert((x > 0 && z > 0) || x >= 0 || z >= 0 ||
			(x < CS_P && z < CS_P) ||
			(x != 0 && z != CS_P - 1) ||
			(x != CS_P - 1 && z != 0), "Invalid coordinates");

		glm::ivec3 offset(0);
		int chunkX = x - 1;
		int chunkZ = z - 1;

		if (z == 0) {
			offset.z = -1; 
			chunkX = x - 1;
			chunkZ = CS - 1;
		}
		else if (z == CS_P - 1) {
			offset.z = 1;
			chunkX = x - 1;
			chunkZ = 0;
		}
		else if (x == 0) {
			offset.x = -1;
			chunkX = CS - 1;
			chunkZ = z - 1;
		}
		else if (x == CS_P - 1) {
			offset.x = 1;
			chunkX = 0;
			chunkZ = z - 1;
		}

		std::shared_ptr<const ChunkType> chunk = world.GetChunk(chunkLocation + offset);
		if (chunk) {
			return chunk->GetColumnRow(chunkX, chunkZ);
		}

		return 0;
	}

	static QuadMeshData _CompressQuadData(uint8_t x, uint8_t y, uint8_t z, uint8_t w, uint8_t h, uint8_t dir)
	{

		//std::cout << "Face: " << (int)dir << " pos: (" << (int)x << "," << (int)y << "," << (int)z << ") "
		//		  << "size: (" << (int)w << "x" << (int)h << ")\n";
		QuadMeshData ret = 0;
		ret |= (x & 0x1F) << 0;
		ret |= (y & 0x1F) << 5;
		ret |= (z & 0x1F) << 10;
		ret |= (w & 0x1F) << 15;
		ret |= (h & 0x1F) << 20;
		ret |= (dir & 0x07) << 25;

		return ret;
	}

	std::unordered_map<RGBAColor, std::array<uintC_t, CS_2>> _SplitVoxelsByColor(uint8_t axis, std::shared_ptr<const ChunkType> chunk)
	{
		// MUST HAPPEN AFTER FACE HULLING STEP
		std::unordered_map<RGBAColor, std::array<uintC_t, CS_2>> data;

		for (uint8_t layer = 0; layer < CS; layer++)
		{
			const int bitsLocation = layer * CS + axis * CS_2;

			for (uint8_t row = 0; row < CS; row++)
			{
				uintC_t& col = m_FaceMasks[row + bitsLocation];

				while (col != 0)
				{
					uint8_t y = GetTrailingZeros(col);
					col &= col - 1;


					//if (data.find(type) == data.end())
					//{
					//	data[type] = std::array<uintC_t, CS_2>{};
					//}
					RGBAColor type;
					switch (axis)
					{
					case 0:
					case 1:
						type = chunk->GetVoxelData(row, y, layer);
						data[type][row + layer * CS] |= uintC_t(1) << y;
						break;
					case 2:
					case 3:
						type = chunk->GetVoxelData(layer, y, row);
						data[type][row + layer * CS] |= uintC_t(1) << y;
						break;
					case 4:
					case 5:
						type = chunk->GetVoxelData(row, y, layer);
						data[type][layer + y * CS] |= uintC_t(1) << row;
						break;
						
					}

				}
			}
		}

		return data;
	}

public:

	template<ChunkProvider<ChunkType> ChunkContainer, VoxelMeshWriter MeshWriter>
	void MeshChunk(const ChunkContainer& chunkGrid, const glm::ivec3& chunkLocation, MeshWriter& out)
	{
		// TODO ideally you wouldnt have to check if the chunk is valid, change to assert if possible
		std::shared_ptr<const ChunkType> chunk = chunkGrid.GetChunk(chunkLocation);
		if (chunk == nullptr || chunk->IsEmpty()) {
			return;
		}

		std::fill(m_FaceMasks.begin(), m_FaceMasks.end(), 0);

		std::shared_ptr<const ChunkType> topChunk = chunkGrid.GetChunk(chunkLocation + glm::ivec3(0, 1, 0));
		std::shared_ptr<const ChunkType> bottomChunk = chunkGrid.GetChunk(chunkLocation - glm::ivec3(0, 1, 0));

		// face hulling
		for (int a = 1; a < CS_P - 1; a++) {
			for (int b = 1; b < CS_P - 1; b++) {
				const uintC_t columnBits = _GetPaddedColumnRowBits(chunkGrid, b, a, chunkLocation);
				const int baIndex = (b - 1) + (a - 1) * CS;
				const int abIndex = (a - 1) + (b - 1) * CS;


				// +ve, -ve z
				m_FaceMasks[baIndex + 0 * CS_2] = (columnBits & ~_GetPaddedColumnRowBits(chunkGrid, b, a - 1, chunkLocation));
				m_FaceMasks[baIndex + 1 * CS_2] = (columnBits & ~_GetPaddedColumnRowBits(chunkGrid, b, a + 1, chunkLocation));

				// +ve, -ve x
				m_FaceMasks[abIndex + 2 * CS_2] = (columnBits & ~_GetPaddedColumnRowBits(chunkGrid, b + 1, a, chunkLocation));
				m_FaceMasks[abIndex + 3 * CS_2] = (columnBits & ~_GetPaddedColumnRowBits(chunkGrid, b - 1, a, chunkLocation));

				//TODO optimize and cleanup
				// +ve, -ve y
				uintC_t postiveYMask = ~(topChunk->GetColumnRow(b-1, a-1) & uintC_t(1) << CS - 1);
				uintC_t negitveYMask = ~(bottomChunk->GetColumnRow(b - 1, a - 1) & (uintC_t(1) << CS - 1) >> CS - 1);

				m_FaceMasks[baIndex + 4 * CS_2] = columnBits & ~(columnBits >> 1) & postiveYMask;
				m_FaceMasks[baIndex + 5 * CS_2] = columnBits & ~(columnBits << 1) & negitveYMask;
			}
		}

		//for (int faces = 0; faces < 6; faces++) {
		//	std::cout << "Face " << faces << std::endl;
		//	for (int x = 0; x < CS; x++) {
		//		std::cout << "layer " << x << std::endl;
		//		for (int y = 0; y < CS; y++) {
		//			std::cout << std::bitset<8>(m_FaceMasks[y + x * CS + faces * CS_2]) << std::endl;
		//		}
		//	
		//	}
		//}


		// Greedy Meshing
		for (uint8_t axis = 0; axis < 6; axis++)
		{
			std::unordered_map<RGBAColor, std::array<uintC_t, CS_2>> data = _SplitVoxelsByColor(axis, chunk);

			for (auto& [type, axisFaceMask] : data)
			{
				for (uint8_t layer = 0; layer < CS; layer++)
				{
					const int bitsLocation = layer * CS;
					for (uint8_t row = 0; row < CS; row++)
					{
						if (axisFaceMask[row + bitsLocation] == 0) continue;
						uint8_t y = 0;

						while (y < CS)
						{
							y += GetTrailingZeros(axisFaceMask[row + bitsLocation] >> y);

							if (y >= CS) break;

							uint8_t h = GetTrailingOnes(axisFaceMask[row + bitsLocation] >> y);

							uintC_t hMask = (h >= CS) ? ~uintC_t(0) : ((uintC_t(1) << h) - 1);
							uintC_t mask = hMask << y;

							uint8_t w = 1;

							while (row + w < CS) {
								// fetch bits spanning height, in the next row
								uintC_t nextRowH = (axisFaceMask[row + w + bitsLocation] >> y) & hMask;
								if (nextRowH != hMask) {
									break; // can no longer expand horizontally
								}

								axisFaceMask[row + w + bitsLocation] &= ~mask;
								w++;
							}

							QuadMeshData quad;
							switch (axis) {
							case 0:
							case 1:
								quad = _CompressQuadData(row, y, layer, w, h, axis);
								break;
							case 2:
							case 3:
								quad = _CompressQuadData(layer, y, row, w, h, axis);
								break;
							case 4:
							case 5:
								quad = _CompressQuadData(y, layer, row, h, w, axis);
								break;
							}

							out.Write(quad, type);


							y += h;

						}
					}
				}
			}
		}
	}

	template<ChunkProvider<ChunkType> ChunkContianer, VoxelMeshWriter MeshWriter>
	void MeshChunkGrid(const ChunkContianer& chunkGrid, MeshWriter& out) const
	{	
		for (const auto& [index, _] : chunkGrid) 
		{
			glm::ivec3 chunkCoords = ChunkGrid<ChunkType>::GetChunkCoords(index);
			meshChunk(chunkGrid, chunkCoords, out);
		}
	}
};


typedef VoxelMesher<Chunk8> VoxelMesher8;
typedef VoxelMesher<Chunk16> VoxelMesher16;
typedef VoxelMesher<Chunk32> VoxelMesher32;

#endif



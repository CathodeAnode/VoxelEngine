#ifndef VOXELMESHER_H
#define VOXELMESHER_H

#include <vector>
#include <iostream>
#include <unordered_map>
#include <stdexcept>

#include <glm/glm.hpp>

#include <bitset>
#include <chrono>

#include "chunk.h"
#include "chunk_grid.h"
#include "voxel_mesh_writer.h"
#include "types.h"
#include "helpers.h"


template<typename ChunkType> class VoxelMesher
{
private:
	using uintC_t = typename ChunkType::ValueType; // TODO change to a more meaningful name

	static constexpr int CS = ChunkType::Size;
	static constexpr int CS_P = CS + 2;
	static constexpr int CS_2 = CS * CS;
	static constexpr int CS_P2 = CS_P * CS_P;
	static constexpr int CS_P3 = CS_P2 * CS_P;

	static uintC_t _GetPaddedColumnRowBits(const ChunkGrid<ChunkType>& world, int x, int z, const glm::ivec3& chunkLocation) {
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

		const ChunkType* chunk = world.getChunk(chunkLocation + offset);
		if (chunk) {
			return chunk->getColumnRow(chunkX, chunkZ);
		}

		return 0;
	}

	static glm::ivec3 _GetAxisIndex(const int axis, const int a, const int b, const int c) {
		glm::ivec3 voxelCoords;
		switch (axis) {
		case 0:
			voxelCoords = glm::ivec3(b, a,c);
			break;
		case 1:
			voxelCoords = glm::ivec3(b, c, a);
			break;
		default:
			voxelCoords = glm::ivec3(c, a, b);
			break;
		}

		return voxelCoords;
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

public:

	// TODO : implement voxel type support for tan tan greedy meshing (faces 0-4)
	template<VoxelMeshWriter MeshWriter>
	void MeshChunk(const ChunkGrid<ChunkType>& chunkGrid, const glm::ivec3& chunkLocation, MeshWriter& out) const
	{
		// TODO ideally you wouldnt have to check if the chunk is valid, change to assert if possible
		const ChunkType* chunk = chunkGrid.getChunk(chunkLocation);
		if (chunk == nullptr || chunk->isEmpty()) {
			return;
		}

		// TODO change to class methods to avoid heap allocations per call (call .clear() here to reset to zeros)
		std::vector<uintC_t> faceMasks(CS_2 * 6, 0);
		std::vector<uint8_t> forwardMerged(CS_2, 0);
		std::vector<uint8_t> rightMerged(CS, 0);

		// face hulling
		for (int a = 1; a < CS_P - 1; a++) {
			for (int b = 1; b < CS_P - 1; b++) {
				const uintC_t columnBits = _GetPaddedColumnRowBits(chunkGrid, b, a, chunkLocation);
				const int baIndex = (b - 1) + (a - 1) * CS;
				const int abIndex = (a - 1) + (b - 1) * CS;


				// +ve, -ve z
				faceMasks[baIndex + 0 * CS_2] = (columnBits & ~_GetPaddedColumnRowBits(chunkGrid, b, a - 1, chunkLocation));
				faceMasks[baIndex + 1 * CS_2] = (columnBits & ~_GetPaddedColumnRowBits(chunkGrid, b, a + 1, chunkLocation));

				// +ve, -ve x
				faceMasks[abIndex + 2 * CS_2] = (columnBits & ~_GetPaddedColumnRowBits(chunkGrid, b + 1, a, chunkLocation));
				faceMasks[abIndex + 3 * CS_2] = (columnBits & ~_GetPaddedColumnRowBits(chunkGrid, b - 1, a, chunkLocation));

				// TODO: fix me, adjacent chunk padding
				// +ve, -ve y
				faceMasks[baIndex + 4 * CS_2] = columnBits & ~(columnBits >> 1);
				faceMasks[baIndex + 5 * CS_2] = columnBits & ~(columnBits << 1);
			}
		}


		//for (int faces = 0; faces < 6; faces++) {
		//	std::cout << "Face " << faces << std::endl;
		//	for (int x = 0; x < CS; x++) {
		//		std::cout << "layer " << x << std::endl;
		//		for (int y = 0; y < CS; y++) {
		//			std::cout << std::bitset<8>(faceMasks[y + x * CS + faces * CS_2]) << std::endl;
		//		}
		//	
		//	}
		//}


		// TAN TAN Greedy Mesher (face 0-3)
		for (uint8_t axis = 0; axis < 4; axis++)
		{
			for (uint8_t layer = 0; layer < CS; layer++)
			{
				const int bitsLocation = layer * CS + axis * CS_2;
				for (uint8_t row = 0; row < CS; row++) 
				{
					if (faceMasks[row + bitsLocation] == 0) continue;
					uint8_t y = 0;

					while (y < CS) 
					{
						y += GetTrailingZeros(faceMasks[row + bitsLocation] >> y);

						if (y >= CS) break;

						uint8_t h = GetTrailingOnes(faceMasks[row + bitsLocation] >> y);

						uintC_t hMask = (h >= CS) ? ~uintC_t(0) : ((uintC_t(1) << h) - 1);
						uintC_t mask = hMask << y;

						uint8_t w = 1;

						while (row + w < CS) {
							// fetch bits spanning height, in the next row
							uintC_t nextRowH = (faceMasks[row + w + bitsLocation] >> y) & hMask;
							if (nextRowH != hMask) {
								break; // can no longer expand horizontally
							}

							faceMasks[row + w + bitsLocation] &= ~mask;
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
						}

						out.Write(quad, 0);


						y += h;

					}

					
					
				}
			}
		}


		//// Greedy meshing face 0-3
		//for (int face = 0; face < 4; face++)
		//{
		//	const int axis = face / 2;
		//	
		//	for (int layer = 0; layer < CS; layer++) {
		//		const int bitsLocation = layer * CS + face * CS_2;

		//		for (int forward = 0; forward < CS; forward++) {
		//			uintC_t bitsHere = faceMasks[forward + bitsLocation];
		//			if (bitsHere == 0) {
		//				continue;
		//			}

		//			const uintC_t bitsNext = forward + 1 < CS ? faceMasks[(forward + 1) + bitsLocation] : 0;
		//			uint8_t _rightMerged = 1;
		//			while (bitsHere) {
		//				unsigned long bitPos = GetTrailingZeros(bitsHere);
		//				
		//				const uint16_t type = chunk->getVoxelData(_GetAxisIndex(axis, forward, bitPos, layer)); // TODO change to Color type
		//				uint8_t& forwardMergedRef = forwardMerged[bitPos];

		//				// (bitsNext >> bitPos & 1) check does two things
		//				// 1. is shift next column of voxels by the trailling zeros
		//				// 2. check that bitsNext is not 0
		//				if ((bitsNext >> bitPos & 1) &&
		//					type == chunk->getVoxelData(_GetAxisIndex(axis, forward + 1, bitPos, layer))) {
		//					forwardMergedRef++;
		//					bitsHere &= ~(1ull << bitPos);
		//					continue;
		//				}

		//				for (int right = bitPos + 1; right < CS; right++) {
		//					if (!(bitsHere >> right & 1) || forwardMergedRef != forwardMerged[right] ||
		//						type != chunk->getVoxelData(_GetAxisIndex(axis, forward, right, layer))) {
		//						break;
		//					}

		//					forwardMerged[right] = 0;
		//					_rightMerged++;
		//				}

		//				bitsHere &= ~((1ull << (bitPos + _rightMerged)) - 1);

		//				const uint8_t meshFront = forward - forwardMergedRef;
		//				const uint8_t meshLeft = bitPos;
		//				const uint8_t meshUp = layer;

		//				const uint8_t meshWidth = _rightMerged;
		//				const uint8_t meshLength = forwardMergedRef + 1;

		//				forwardMergedRef = 0;
		//				_rightMerged = 1;

	

		//				//switch (face) {
		//				//case 0:
		//				//case 1:
		//				//	mesh.addQuad(meshFront, meshLeft, meshUp, meshLength, meshWidth, face, type);
		//				//	break;
		//				//case 2:
		//				//case 3:
		//				//	mesh.addQuad(meshUp, meshFront, meshLeft, meshLength, meshWidth, face, type);
		//				//	break;
		//				//}


		//				QuadMeshData quad;
		//				switch (face) {
		//					case 0:
		//					case 1:
		//						quad = _CompressQuadData(meshLeft, meshFront, meshUp, meshWidth, meshLength, face);
		//						break;
		//					case 2:
		//					case 3:
		//						quad = _CompressQuadData(meshUp, meshFront, meshLeft, meshWidth, meshLength, face);
		//						break;
		//				}

		//				out.Write(quad, 0);

		//			}
		//		}
		//	}
		//}

		// Greedy meshing faces 4-5
		for (int face = 4; face < 6; face++) 
		{
			const int axis = face / 2;

			for (int forward = 0; forward < CS; forward++) 
			{
				const int bitsLocation = forward * CS + face * CS_2;
				const int bitsForwardLocation = (forward + 1) * CS + face * CS_2;

				for (int right = 0; right < CS; right++)
				{
					uintC_t bitsHere = faceMasks[right + bitsLocation];
					if (bitsHere == 0) continue;

					const uintC_t bitsForward = forward < CS - 1 ? faceMasks[right + bitsForwardLocation] : 0;
					const uintC_t bitsRight = right < CS - 1 ? faceMasks[right + 1 + bitsLocation] : 0;
					const int rightCS = right * CS;

					while (bitsHere) {
						unsigned long bitPos = GetTrailingZeros(bitsHere);

						bitsHere &= ~(uintC_t(1) << bitPos);

						const uint16_t type = chunk->getVoxelData(_GetAxisIndex(axis, right , forward, bitPos));
						uint8_t& forwardMergedRef = forwardMerged[rightCS + bitPos];
						uint8_t& rightMergedRef = rightMerged[bitPos];

						if (rightMergedRef == 0 && (bitsForward >> bitPos & 1) &&
							type == chunk->getVoxelData(_GetAxisIndex(axis, right, forward + 1, bitPos))) 
						{
							forwardMergedRef++;
							continue;
						}

						if ((bitsRight >> bitPos & 1) && forwardMergedRef == forwardMerged[(rightCS + CS) + bitPos]
							&& type == chunk->getVoxelData(_GetAxisIndex(axis, right + 1, forward, bitPos))) 
						{
							forwardMergedRef = 0;
							rightMergedRef++;
							continue;
						}

						const uint8_t meshLeft = right - rightMergedRef;
						const uint8_t meshFront = forward - forwardMergedRef;
						const uint8_t meshUp = bitPos;

						const uint8_t meshWidth = 1 + rightMergedRef;
						const uint8_t meshLength = 1 + forwardMergedRef;

						forwardMergedRef = 0;
						rightMergedRef = 0;

						QuadMeshData quad = _CompressQuadData(meshLeft, meshUp, meshFront, meshWidth, meshLength, face);
						out.Write(quad, type);

					}
				}
			}
		}
		
	}

	template<VoxelMeshWriter MeshWriter>
	void MeshChunkGrid(const ChunkGrid<ChunkType>& chunkGrid, MeshWriter& out) const
	{	
		for (const auto& [index, _] : chunkGrid) 
		{
			glm::ivec3 chunkCoords = ChunkGrid<ChunkType>::getChunkCoords(index);
			meshChunk(chunkGrid, chunkCoords, out);
		}
	}
};


typedef VoxelMesher<Chunk8> VoxelMesher8;
typedef VoxelMesher<Chunk16> VoxelMesher16;
typedef VoxelMesher<Chunk32> VoxelMesher32;

#endif



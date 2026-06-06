#ifndef VOXEL_MESHER_TPP
#define VOXEL_MESHER_TPP


#include "voxel_mesher.h"
#include "voxel_mesher.h"

template<typename ChunkType>
VoxelMesher<ChunkType>::VoxelMesher()
{
	PROFILE_FUNCTION();
	LOG_INFO(EngineSystem::VOXEL_MESHER, "Initializing Greedy Mesher");
}

template<typename ChunkType>
QuadMeshData VoxelMesher<ChunkType>::_CompressQuadData(uint8_t x, uint8_t y, uint8_t z, uint8_t w, uint8_t h, uint8_t dir)
{
	PROFILE_FUNCTION();

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

template<typename ChunkType>
VoxelMesher<ChunkType>::ColorFaceMasksMap VoxelMesher<ChunkType>::_SplitVoxelsByColor(uint8_t axis, std::shared_ptr<const ChunkType> chunk, FaceVisibilityMasks& faceMasks)
{
	PROFILE_FUNCTION();

	// MUST HAPPEN AFTER FACE HULLING STEP
	ColorFaceMasksMap data;

	for (uint8_t layer = 0; layer < CS; layer++)
	{
		const int bitsLocation = layer * CS + axis * CS_2;

		for (uint8_t row = 0; row < CS; row++)
		{
			VoxelColumnData& col = faceMasks[row + bitsLocation];

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
					type = chunk->GetVoxelColorAt(row, y, layer);
					data[type][row + layer * CS] |= VoxelColumnData(1) << y;
					break;
				case 2:
				case 3:
					type = chunk->GetVoxelColorAt(layer, y, row);
					data[type][row + layer * CS] |= VoxelColumnData(1) << y;
					break;
				case 4:
				case 5:
					type = chunk->GetVoxelColorAt(row, y, layer);
					data[type][layer + y * CS] |= VoxelColumnData(1) << row;
					break;

				}

			}
		}
	}

	return data;
}

template<typename ChunkType>
template<ChunkProvider<ChunkType> ChunkContainer>
std::vector<VoxelQuad> VoxelMesher<ChunkType>::MeshChunk(const ChunkContainer& chunkContainer, const glm::ivec3& chunkLocation)
{
	PROFILE_FUNCTION();
	FaceVisibilityMasks faceMasks;
	std::vector<VoxelQuad> chunkMesh;
	std::fill(faceMasks.begin(), faceMasks.end(), 0);
	chunkMesh.reserve(CS_2 * 6);

	ChunkData<ChunkType> chunks;
	LOG_DEBUG(EngineSystem::VOXEL_MESHER,
		"Meshing chunk at ({}, {}, {}) in container {}",
		chunkLocation.x,
		chunkLocation.y,
		chunkLocation.z,
		chunkContainer.GetUID());

	std::fill(faceMasks.begin(), faceMasks.end(), 0);
	chunkMesh.clear();

	chunks.GatherData(chunkContainer, chunkLocation);

	// face hulling
	for (int a = 1; a < CS_P - 1; a++) {
		for (int b = 1; b < CS_P - 1; b++) {
			const VoxelColumnData columnBits = chunks.GetPaddedColumnRowBits(b, a);
			const int baIndex = (b - 1) + (a - 1) * CS;
			const int abIndex = (a - 1) + (b - 1) * CS;


			// +ve, -ve z
			faceMasks[baIndex + 0 * CS_2] = (columnBits & ~chunks.GetPaddedColumnRowBits(b, a - 1));
			faceMasks[baIndex + 1 * CS_2] = (columnBits & ~chunks.GetPaddedColumnRowBits(b, a + 1));

			// +ve, -ve x
			faceMasks[abIndex + 2 * CS_2] = (columnBits & ~chunks.GetPaddedColumnRowBits(b + 1, a));
			faceMasks[abIndex + 3 * CS_2] = (columnBits & ~chunks.GetPaddedColumnRowBits(b - 1, a));

			// +ve, -ve y
			const VoxelColumnData postiveBottomBit = chunks.yPos->GetColumnRow(b - 1, a - 1) & VoxelColumnData(1);
			const VoxelColumnData negitiveTopBit = chunks.yNeg->GetColumnRow(b - 1, a - 1) & (VoxelColumnData(1) << (CS - 1));
			VoxelColumnData postiveYMask = ~(postiveBottomBit << (CS - 1));
			VoxelColumnData negitveYMask = ~(negitiveTopBit >> (CS - 1));

			faceMasks[baIndex + 4 * CS_2] = (columnBits & ~(columnBits >> 1)) & postiveYMask;
			faceMasks[baIndex + 5 * CS_2] = (columnBits & ~(columnBits << 1)) & negitveYMask;
		}
	}

	// Greedy Meshing
	for (uint8_t axis = 0; axis < 6; axis++)
	{
		ColorFaceMasksMap data = _SplitVoxelsByColor(axis, chunks.center, faceMasks);

		for (auto& [type, axisFaceMask] : data)
		{
			for (uint8_t layer = 0; layer < CS; layer++)
			{
				const int bitsLocation = layer * CS;
				VoxelColumnData* layerPtr = &axisFaceMask[bitsLocation];
				for (uint8_t row = 0; row < CS; row++)
				{
					VoxelColumnData rowMask = layerPtr[row];
					if (rowMask == 0) continue;
					uint8_t y = 0;

					while (y < CS)
					{
						y += GetTrailingZeros(rowMask >> y);

						if (y >= CS) break;

						uint8_t h = GetTrailingOnes(rowMask >> y);

						VoxelColumnData hMask = (h >= CS) ? ~VoxelColumnData(0) : ((VoxelColumnData(1) << h) - 1);
						VoxelColumnData mask = hMask << y;

						uint8_t w = 1;

						while (row + w < CS)
						{
							// fetch bits spanning height, in the next row
							VoxelColumnData* nextRowPtr = &layerPtr[row + w];

							if (((*nextRowPtr >> y) & hMask) != hMask)
								break; // can no longer expand horizontally

							*nextRowPtr &= ~mask;
							w++;
						}

						QuadMeshData quad;
						switch (axis)
						{
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

						LOG_TRACE(EngineSystem::VOXEL_MESHER,
							"Chunk quad upload -> chunk=({},{},{}), size={}x{}, pos=({},{},{}), axis={}, color=0x{:08X}"
							, chunkLocation.x, chunkLocation.y, chunkLocation.z
							, w, h
							, row, y, layer
							, axis
							, type);

						chunkMesh.push_back({ quad, type });


						y += h;

					}
				}
			}
		}
	}

	return chunkMesh;
}

template<typename ChunkType>
template<ChunkProvider<ChunkType> ChunkContianer>
std::vector<VoxelQuad>  VoxelMesher<ChunkType>::MeshChunkGrid(const ChunkContianer& chunkContainer)
{
	LOG_DEBUG(EngineSystem::VOXEL_MESHER,
		"Meshing chunk container {}",
		chunkContainer.GetUID());

	LOG_ERROR(EngineSystem::VOXEL_MESHER,
		"MeshChunkGrid Not Implemented");

	//for (const auto& [index, _] : chunkContainer)
	//{
	//	glm::ivec3 chunkCoords = chunkContainer<ChunkType>::GetChunkCoords(index);
	//	meshChunk(chunkContainer, chunkCoords, out);
	//}
}

#endif
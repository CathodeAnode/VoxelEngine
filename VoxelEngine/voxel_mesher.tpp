#ifndef VOXEL_MESHER_TPP
#define VOXEL_MESHER_TPP


#include "voxel_mesher.h"
#include "voxel_mesher.h"

template<typename ChunkType>
template<ChunkProvider<ChunkType> ChunkContainer>
VoxelMesher<ChunkType>::VoxelColumnData VoxelMesher<ChunkType>::_GetPaddedColumnRowBits(const ChunkContainer& world, int x, int z, const glm::ivec3& chunkLocation)
{
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


template<typename ChunkType>
QuadMeshData VoxelMesher<ChunkType>::_CompressQuadData(uint8_t x, uint8_t y, uint8_t z, uint8_t w, uint8_t h, uint8_t dir)
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

template<typename ChunkType>
VoxelMesher<ChunkType>::ColorFaceMasksMap VoxelMesher<ChunkType>::_SplitVoxelsByColor(uint8_t axis, std::shared_ptr<const ChunkType> chunk)
{
	// MUST HAPPEN AFTER FACE HULLING STEP
	ColorFaceMasksMap data;

	for (uint8_t layer = 0; layer < CS; layer++)
	{
		const int bitsLocation = layer * CS + axis * CS_2;

		for (uint8_t row = 0; row < CS; row++)
		{
			VoxelColumnData& col = m_FaceMasks[row + bitsLocation];

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
template<ChunkProvider<ChunkType> ChunkContainer, VoxelMeshWriter MeshWriter>
void VoxelMesher<ChunkType>::MeshChunk(const ChunkContainer& chunkGrid, const glm::ivec3& chunkLocation, MeshWriter& out)
{
	std::shared_ptr<const ChunkType> chunk = chunkGrid.GetChunk(chunkLocation);
	assert(chunk != nullptr);

	if (chunk->IsEmpty()) 
	{
		return;
	}

	std::fill(m_FaceMasks.begin(), m_FaceMasks.end(), 0);

	std::shared_ptr<const ChunkType> topChunk = chunkGrid.GetChunk(chunkLocation + glm::ivec3(0, 1, 0));
	std::shared_ptr<const ChunkType> bottomChunk = chunkGrid.GetChunk(chunkLocation - glm::ivec3(0, 1, 0));

	// face hulling
	for (int a = 1; a < CS_P - 1; a++) {
		for (int b = 1; b < CS_P - 1; b++) {
			const VoxelColumnData columnBits = _GetPaddedColumnRowBits(chunkGrid, b, a, chunkLocation);
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
			VoxelColumnData postiveYMask = ~(topChunk->GetColumnRow(b - 1, a - 1) & VoxelColumnData(1) << CS - 1);
			VoxelColumnData negitveYMask = ~(bottomChunk->GetColumnRow(b - 1, a - 1) & (VoxelColumnData(1) << CS - 1) >> CS - 1);

			m_FaceMasks[baIndex + 4 * CS_2] = columnBits & ~(columnBits >> 1) & postiveYMask;
			m_FaceMasks[baIndex + 5 * CS_2] = columnBits & ~(columnBits << 1) & negitveYMask;
		}
	}

	// Greedy Meshing
	for (uint8_t axis = 0; axis < 6; axis++)
	{
		ColorFaceMasksMap data = _SplitVoxelsByColor(axis, chunk);

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

						VoxelColumnData hMask = (h >= CS) ? ~VoxelColumnData(0) : ((VoxelColumnData(1) << h) - 1);
						VoxelColumnData mask = hMask << y;

						uint8_t w = 1;

						while (row + w < CS) {
							// fetch bits spanning height, in the next row
							VoxelColumnData nextRowH = (axisFaceMask[row + w + bitsLocation] >> y) & hMask;
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

template<typename ChunkType>
template<ChunkProvider<ChunkType> ChunkContianer, VoxelMeshWriter MeshWriter>
void VoxelMesher<ChunkType>::MeshChunkGrid(const ChunkContianer& chunkGrid, MeshWriter& out)
{
	for (const auto& [index, _] : chunkGrid)
	{
		glm::ivec3 chunkCoords = ChunkGrid<ChunkType>::GetChunkCoords(index);
		meshChunk(chunkGrid, chunkCoords, out);
	}
}

#endif
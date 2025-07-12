#ifndef WORLD_H
#define WORLD_H

#include <glm/glm.hpp>

#include "chunk_grid.h"
#include "voxel_mesher.h"
#include "voxel_renderer.h"

template<typename ChunkType> class World {
private:
	ChunkGrid<ChunkType> chunks;
	VoxelMesher<ChunkType> mesher;
	VoxelRenderer renderer;
	std::unordered_map<uint64_t, Page> GPUChunkMeshIndex;
	int maxChunks;


public:

	/**
	 * Creates a chunks spanning from -xz size/2 to +xz size/2
	 *
	 *
	 * @param worldSize Size of the flat world
	 */
	World(int worldSize = 0) : maxChunks(worldSize * worldSize) {
		renderer.allocBuffers(100 * 1024 * 1024, 11000, 11000);
		//renderer.toggleDrawLines();

		for (int x = -worldSize /2; x <= worldSize /2; x++) {
			for (int z = -worldSize /2; z <= worldSize /2; z++) {
				chunks.addChunk(ChunkType(true), glm::ivec3(x, -1, z));
			}
		}
		chunks.getChunk(glm::ivec3(0, -1, 0))->toggleBit(4, 7, 4);
		chunks.getChunk(glm::ivec3(0, -1, 0))->toggleBit(4, 7, 3);
		chunks.getChunk(glm::ivec3(0, -1, 0))->toggleBit(3, 7, 4);
		chunks.getChunk(glm::ivec3(0, -1, 0))->toggleBit(3, 7, 3);
		chunks.getChunk(glm::ivec3(1, -1, 1))->toggleBit(4, 4, 7);
		//chunks.getChunk(glm::ivec3(0, -1, 0))->toggleBit(4, 4, 4);

		ChunkType* tester = chunks.getChunk(glm::ivec3(1, -1, 0));
		for (int x = 0; x < 8; x++) {
			for (int y = 0; y < 8; y++) {
				for (int z = 0; z < 8; z++) {
					tester->toggleBit(x, y, z);
				}
			}
		}

		std::vector<uint32_t> worldData;
		ChunkQuads mesh;

		int offset = 0;
		std::vector<glm::vec3> pos;
		std::vector<Page> test;
		for (const auto& [index, _] : chunks) {
			glm::ivec3 coords = ChunkGrid<ChunkType>::getChunkCoords(index);
			mesh = mesher.meshChunk(chunks, coords);
			worldData.insert(worldData.end(), mesh.quadData.begin(), mesh.quadData.end());

			std::cout << "(" << coords.x << ", " << coords.y << ", " << coords.z << "): ";
			std::cout << offset << " " << mesh.quadData.size() << std::endl;
			GPUChunkMeshIndex[index] = Page(offset, mesh.quadData.size());

			test.push_back(Page(offset, mesh.quadData.size()));
			pos.push_back(glm::vec3(coords.x, coords.y, coords.z) * (float)ChunkType::Size);

			offset += mesh.quadData.size();
		}

		renderer.uploadData(worldData.data(), offset);

		//std::vector<glm::vec3> pos = { glm::vec3(0, -1, 0) * (float)ChunkType::Size, glm::vec3(1, -1, 0) * (float)ChunkType::Size, glm::vec3(0, -1, 1) * (float)ChunkType::Size };
		renderer.uploadPositionData(pos);
		//renderer.uploadPositionData({ glm::vec3(0, 0, 16), glm::vec3(0, 0, 0)});


		//renderer.uploadIndirectCommands({ GPUChunkMeshIndex[ChunkGrid<ChunkType>::getChunkIndex(0, -1, 0)],
		//								  GPUChunkMeshIndex[ChunkGrid<ChunkType>::getChunkIndex(1, -1, 0)],
		//								  GPUChunkMeshIndex[ChunkGrid<ChunkType>::getChunkIndex(0, -1, 1)] });

		//renderer.uploadIndirectCommands({ GPUChunkMeshIndex[ChunkGrid<ChunkType>::getChunkIndex(3,-1,3)],
		//								  GPUChunkMeshIndex[ChunkGrid<ChunkType>::getChunkIndex(3, -1, 2)]});
		renderer.uploadIndirectCommands(test);


		//for (const auto& [index, cmd] : GPUDrawCommand) {
		//	glm::ivec3 coords = ChunkGrid<ChunkType>::getChunkCoords(index);
		//	std::cout << "(" << coords.x << ", " << coords.y << ", " << coords.z << ") " << cmd.index << ", " << cmd.size << std::endl;
		//}
	}

	World(const char* filePath);

	void render() {
		renderer.render();
	}

	void setBlock(glm::ivec3 coords, uint16_t type);
	void removeBlock(glm::ivec3 coords);

	void addChunk(glm::ivec3 chunkCoords, ChunkType chunk);
	void removeChunk(glm::ivec3 chunkCoords);

	void saveModel(const char* filePath);

};

typedef World<Chunk8> World8;
typedef World<Chunk16> World16;
typedef World<Chunk32> World32;

#endif

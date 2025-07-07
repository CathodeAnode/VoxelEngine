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
	std::unordered_map<uint64_t, IndirectDrawCommand> GPUChunkMeshIndex;
	int maxChunks;


public:

	/**
	 * Creates a chunks spanning from -xz size/2 to +xz size/2
	 *
	 *
	 * @param worldSize Size of the flat world
	 */
	World(int worldSize = 0) : maxChunks(worldSize * worldSize) {
		renderer.allocBuffers(100 * 1024 * 1024, 100, 500);

		for (int x = -worldSize /2; x < worldSize /2; x++) {
			for (int z = -worldSize /2; z < worldSize /2; z++) {
				chunks.addChunk(ChunkType(true), glm::ivec3(x, -1, z));
			}
		}
		chunks.getChunk(glm::ivec3(0, -1, 0))->toggleBit(4, 7, 4);

		std::vector<uint32_t> worldData;
		ChunkQuads mesh;

		int offset = 0;
		for (const auto& [index, _] : chunks) {
			mesh = mesher.meshChunk(chunks, ChunkGrid<ChunkType>::getChunkCoords(index));
			if (index == 4398044413952) {
				std::cout << mesh.quadData.size() << std::endl;
			}
			worldData.insert(worldData.end(), mesh.quadData.begin(), mesh.quadData.end());
			GPUChunkMeshIndex[index] = IndirectDrawCommand(offset, mesh.quadData.size());
			offset += mesh.quadData.size();
		}

		renderer.uploadData(worldData.data(), offset);

		std::vector<glm::vec3> pos = { glm::vec3(0, -1, 0) * (float)ChunkType::Size, glm::vec3(1, -1, 0) * (float)ChunkType::Size };
		renderer.uploadPositionData(pos.data(), 2);


		renderer.uploadIndirectCommands({ GPUChunkMeshIndex[ChunkGrid<ChunkType>::getChunkIndex(0, -1, 0)],
										  GPUChunkMeshIndex[ChunkGrid<ChunkType>::getChunkIndex(1, -1, 0)] });


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

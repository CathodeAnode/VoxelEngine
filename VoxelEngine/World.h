#ifndef WORLD_H
#define WORLD_H

#include <glm/glm.hpp>

#include "chunk_grid.h"
#include "voxel_mesher.h"
#include "voxel_renderer.h"


#define QUAD_FACES_PER_CHUNK 200u
#define NUMBER_OF_CHUNKS 8192

template<typename ChunkType> class World {
private:
	ChunkGrid<ChunkType>* chunks;
	VoxelMesher<ChunkType>* mesher;
	VoxelRenderer* renderer;
	std::unordered_map<uint64_t, Page> GPUChunkMeshIndex;

	int worldSize; // NxNxN chunks
	int renderDistance;
	glm::ivec3 lastPlayerGridCoords;

	


public:

	/**
	 * Creates a chunks spanning from -xz size/2 to +xz size/2
	 *
	 *
	 * @param worldSize Size of the flat world
	 */
	World(int _worldSize, unsigned int _renderDistance) : worldSize(_worldSize * _worldSize * _worldSize), renderDistance(_renderDistance) {
		chunks = new ChunkGrid<ChunkType>();
		mesher = new VoxelMesher<ChunkType>();
		renderer = new VoxelRenderer(QUAD_FACES_PER_CHUNK * NUMBER_OF_CHUNKS, pow(renderDistance, 3));

		lastPlayerGridCoords = glm::ivec3(MAX_GRID_INT, MAX_GRID_INT, MAX_GRID_INT) + 100;
	}

	~World() {
		delete chunks;
		delete mesher;
		delete renderer;
	}

	World(const char* filePath);

	void updateVisibleChunksByDistance(const glm::vec3& playerWorldCoords) {

		// step 1: convert player world coordinates to grid coordinates
		const int ChunkSize = ChunkType::Size;

		int chunkX = playerWorldCoords.x / ChunkSize;
		int chunkY = playerWorldCoords.y / ChunkSize;
		int chunkZ = playerWorldCoords.z / ChunkSize;
		const glm::ivec3 playerGridCoords(chunkX, chunkY, chunkZ);
		//std::cout << chunkX << ", " << chunkY << ", " << chunkZ << std::endl;

		// step 2: check if player grid coords has changed since last call
		if (playerGridCoords == lastPlayerGridCoords) {
			return; //exit early
		}

		// step 3: compute chunks to be rendered around player in sphereical volume
		const int renderDistRadius_2 = renderDistance * renderDistance;

		std::vector<Page> GPUChunkDataLocations;
		std::vector<glm::vec3> chunkPositions;

		for (int x = -renderDistance; x <= renderDistance; x++) {
			for (int y = -renderDistance; y <= renderDistance; y++) {
				for (int z = -renderDistance; z <= renderDistance; z++) {
					if (x * x + y * y + z * z > renderDistRadius_2) continue; // outside sphere

					glm::ivec3 chunkCoords = playerGridCoords + glm::ivec3(x, y, z); // relative to player
					//std::cout << chunkCoords.x << ", " << chunkCoords.y << ", " << chunkCoords.z << std::endl;
					uint64_t index = ChunkGrid<ChunkType>::getChunkIndex(chunkCoords);

					if (chunks->getChunk(chunkCoords) == nullptr) continue;

					// if not already meshed, mesh chunk (conditional should never be executed in theory)
					if (!GPUChunkMeshIndex.contains(index)) {
						const int dataBufferSize = renderer->getDataBufferSize();
						const ChunkQuads mesh = mesher->meshChunk(*chunks, chunkCoords);
						GPUChunkMeshIndex[index] = Page(dataBufferSize, mesh.quadData.size());
						renderer->addData(mesh.quadData);
					}

					const Page& page = GPUChunkMeshIndex[index];
					GPUChunkDataLocations.push_back(page);

					chunkPositions.push_back(glm::vec3(chunkCoords) * (float)ChunkSize);
				}
			}
		}
		lastPlayerGridCoords = playerGridCoords;

		// step 4: upload data to gpu
		renderer->uploadIndirectCommands(GPUChunkDataLocations);
		renderer->uploadPositionData(chunkPositions);
	}

	void render() {
		renderer->render();
	}

	void setBlock(const glm::ivec3& coords, uint16_t type);
	void removeBlock(const glm::ivec3& coords);

	void addChunk(const glm::ivec3& chunkCoords, const ChunkType& chunk, bool doMesh = false) {
		chunks->addChunk(chunk, chunkCoords);
	}
	void removeChunk(const glm::ivec3& chunkCoords);

	void meshWorld() {
		ChunkQuads mesh;
		std::vector<uint32_t> worldData;

		int offset = 0;
		for (const auto& [index, _] : *chunks) {
			glm::ivec3 coords = ChunkGrid<ChunkType>::getChunkCoords(index);
			mesh = mesher->meshChunk(*chunks, coords);
			GPUChunkMeshIndex[index] = Page(offset, mesh.quadData.size());
			offset += mesh.quadData.size();
			worldData.insert(worldData.end(), mesh.quadData.begin(), mesh.quadData.end());
		}

		renderer->uploadData(worldData);
	}

	void saveModel(const char* filePath);

};

typedef World<Chunk8> World8;
typedef World<Chunk16> World16;
typedef World<Chunk32> World32;

#endif

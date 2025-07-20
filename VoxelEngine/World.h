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
	ChunkGrid<ChunkType> chunks;
	VoxelMesher<ChunkType> mesher;
	VoxelRenderer renderer;

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
	World(int _worldSize, unsigned int _renderDistance) 
		: worldSize(_worldSize * _worldSize * _worldSize)
		, renderDistance(_renderDistance)
	{
		lastPlayerGridCoords = glm::ivec3(MAX_GRID_INT, MAX_GRID_INT, MAX_GRID_INT) + 100;
		renderer.Init(QUAD_FACES_PER_CHUNK* NUMBER_OF_CHUNKS, pow(renderDistance, 3));

	}

	World(const char* filePath);

	void updateVisibleChunksByDistance(const glm::vec3& playerWorldCoords) {
		// step 1: convert player world coordinates to grid coordinates
		const int ChunkSize = ChunkType::Size;

		int chunkX = playerWorldCoords.x / ChunkSize;
		int chunkY = playerWorldCoords.y / ChunkSize;
		int chunkZ = playerWorldCoords.z / ChunkSize;
		const glm::ivec3 playerGridCoords(chunkX, chunkY, chunkZ);

		// step 2: check if player grid coords has changed since last call
		if (playerGridCoords == lastPlayerGridCoords) {
			return; //exit early
		}

		std::cout << chunkX << ", " << chunkY << ", " << chunkZ << std::endl;

		// step 3: compute chunks to be rendered around player in sphereical volume
		const int renderDistRadius_2 = renderDistance * renderDistance;

		const size_t maxChunksRenderedPerFrame = pow(renderDistance, 3);
		DrawArraysIndirectCommand* cmds = renderer.GetDrawCommandsWritePtr();
		glm::vec4* paddedWorldPosition = renderer.GetPositionDataWritePtr();


		size_t chunksRendered = 0;
		for (int x = -renderDistance; x <= renderDistance; x++) {
			for (int y = -renderDistance; y <= renderDistance; y++) {
				for (int z = -renderDistance; z <= renderDistance; z++) {
					if (x * x + y * y + z * z > renderDistRadius_2) continue; // outside sphere

					glm::ivec3 chunkCoords = playerGridCoords + glm::ivec3(x, y, z); // relative to player
					uint64_t encodedChunkindex = ChunkGrid<ChunkType>::GetEncodedChunkCoords(chunkCoords);

					if (chunks.getChunk(encodedChunkindex) == nullptr) continue;

					Page pageOffset = renderer.GetDataPageOffsets(encodedChunkindex);
					if (pageOffset.id == 0) {
						ChunkQuads mesh = mesher.meshChunk(chunks, chunkCoords);
						renderer.UploadMesh(mesh.quadData, encodedChunkindex);
						pageOffset = renderer.GetDataPageOffsets(encodedChunkindex);
					}

					cmds->count = 4;
					cmds->first = 0;
					cmds->baseInstance = pageOffset.index;
					cmds->instanceCount = pageOffset.size;

					paddedWorldPosition->x = chunkCoords.x * ChunkSize;
					paddedWorldPosition->y = chunkCoords.y * ChunkSize;
					paddedWorldPosition->z = chunkCoords.z * ChunkSize;
					paddedWorldPosition->w = 0; 
					
					cmds++;
					paddedWorldPosition++;
					chunksRendered++;
				}
			}
		}

		renderer.CompleteBuffersWrite(chunksRendered);
		lastPlayerGridCoords = playerGridCoords;
	}

	void render() {
		renderer.render();
	}

	void setBlock(const glm::ivec3& coords, uint16_t type);
	void removeBlock(const glm::ivec3& coords) {
		chunks.toggleBlock(coords);
		
		// TODO: remesh chunk
	}

	void addChunk(const glm::ivec3& chunkCoords, const ChunkType& chunk, bool doMesh = false) {
		chunks.addChunk(chunk, chunkCoords);
	}
	void removeChunk(const glm::ivec3& chunkCoords);

	void meshWorld() {
		ChunkQuads mesh;

		int offset = 0;
		for (const auto& [index, _] : chunks) {
			glm::ivec3 coords = ChunkGrid<ChunkType>::getChunkCoords(index);
			mesh = mesher.meshChunk(chunks, coords);
			renderer.UploadMesh(mesh.quadData, index);
		}

	}

	void saveModel(const char* filePath);

	inline ChunkGrid<ChunkType> getGrid() const { return chunks; };

};

typedef World<Chunk8> World8;
typedef World<Chunk16> World16;
typedef World<Chunk32> World32;

#endif

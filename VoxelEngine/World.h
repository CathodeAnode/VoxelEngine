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
		const int chunkSize = ChunkType::Size;

		int chunkX = floor(playerWorldCoords.x / (float)chunkSize);
		int chunkY = floor(playerWorldCoords.y / (float)chunkSize);
		int chunkZ = floor(playerWorldCoords.z / (float)chunkSize);
		const glm::ivec3 playerGridCoords(chunkX, chunkY, chunkZ);

		// step 2: check if player grid coords has changed since last call
		if (playerGridCoords == lastPlayerGridCoords) {
			return; //exit early
		}


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

					paddedWorldPosition->x = chunkCoords.x * chunkSize;
					paddedWorldPosition->y = chunkCoords.y * chunkSize;
					paddedWorldPosition->z = chunkCoords.z * chunkSize;
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
	void removeBlock(const glm::ivec3& voxelWorldCoords) {
		
		int ChunkSize = ChunkType::Size;

		int chunkX = floor(voxelWorldCoords.x / (float)ChunkSize);
		int chunkY = floor(voxelWorldCoords.y / (float)ChunkSize);
		int chunkZ = floor(voxelWorldCoords.z / (float)ChunkSize);
		glm::ivec3 chunkCoords(chunkX, chunkY, chunkZ);
		uint64_t encodedChunkindex = ChunkGrid<ChunkType>::GetEncodedChunkCoords(chunkCoords);
		std::cout << "Chunk Coordinates: " << chunkX << ", " << chunkY << ", " << chunkZ << std::endl;

		ChunkType* chunk = chunks.getChunk(encodedChunkindex);

		if (!chunk) {
			return;
		}

		int xC = ((voxelWorldCoords.x % ChunkSize) + ChunkSize) % ChunkSize;
		int yC = ((voxelWorldCoords.y % ChunkSize) + ChunkSize) % ChunkSize;
		int zC = ((voxelWorldCoords.z % ChunkSize) + ChunkSize) % ChunkSize;
		std::cout << "Voxel (local): " << xC << ", " << yC << ", " << zC << std::endl;
		chunk->toggleBit(xC, yC, zC);
		
		// re-mesh chunk
		ChunkQuads mesh = mesher.meshChunk(chunks, chunkCoords);
		renderer.UpdateMesh(mesh.quadData, encodedChunkindex);

		// re-mesh neighbooring chunks if voxel remove was on edge of chunk
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

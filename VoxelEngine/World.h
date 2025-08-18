#ifndef WORLD_H
#define WORLD_H

#include <glm/glm.hpp>

#include "chunk_grid.h"
#include "voxel_mesher.h"
#include "voxel_renderer.h"


#define QUAD_FACES_PER_CHUNK 200u
#define NUMBER_OF_CHUNKS 8192

template<typename ChunkType> class World {
public:
	World(int _worldSize, unsigned int _renderDistance) 
		: m_WorldSize(_worldSize * _worldSize * _worldSize)
		, m_RenderDistance(_renderDistance)
	{
		m_LastPlayerGridCoords = glm::ivec3(MAX_GRID_INT, MAX_GRID_INT, MAX_GRID_INT) + 100;
		m_MaxRenderableChunks = pow(m_RenderDistance, 3);
		m_Renderer.Init(QUAD_FACES_PER_CHUNK* NUMBER_OF_CHUNKS, m_MaxRenderableChunks);

	}

	World(const char* filePath);

	void UpdateVisibleChunksByDistance(const glm::vec3& playerWorldCoords) {
		// step 1: convert player world coordinates to grid coordinates
		const int chunkSize = ChunkType::Size;

		const glm::ivec3 playerGridCoords(
			floor(playerWorldCoords.x / (float)chunkSize), 
			floor(playerWorldCoords.y / (float)chunkSize),
			floor(playerWorldCoords.z / (float)chunkSize)
		);

		// step 2: check if player grid coords has changed since last call
		if (playerGridCoords == m_LastPlayerGridCoords) {
			return; //exit early
		}


		// step 3: compute m_Chunks to be rendered around player in sphereical volume
		const int renderDistRadius_2 = m_RenderDistance * m_RenderDistance;

		const size_t maxChunksRenderedPerFrame = pow(m_RenderDistance, 3);
		DrawArraysIndirectCommand* cmds = m_Renderer.GetDrawCommandsWritePtr();
		glm::vec4* paddedWorldPosition = m_Renderer.GetPositionDataWritePtr();


		m_ChunksRendered = 0;
		for (int x = -m_RenderDistance; x <= m_RenderDistance; x++) {
			for (int y = -m_RenderDistance; y <= m_RenderDistance; y++) {
				for (int z = -m_RenderDistance; z <= m_RenderDistance; z++) {
					if (x * x + y * y + z * z > renderDistRadius_2) continue; // outside sphere

					glm::ivec3 chunkCoords = playerGridCoords + glm::ivec3(x, y, z); // relative to player
					uint64_t encodedChunkCoords = ChunkGrid<ChunkType>::EncodeChunkCoords(chunkCoords);

					if (m_Chunks.getChunk(encodedChunkCoords) == nullptr) continue;

					Page pageOffset = m_Renderer.GetDataPageOffsets(m_ChunkCoordsPageID[encodedChunkCoords]);
					if (pageOffset.IsNull()) {
						ChunkQuads mesh = m_Mesher.meshChunk(m_Chunks, chunkCoords);
						const size_t pageId = m_Renderer.UploadMesh(mesh.quadData);
						pageOffset = m_Renderer.GetDataPageOffsets(m_ChunkCoordsPageID[encodedChunkCoords]);
					}

					if (m_ChunksRendered >= m_MaxRenderableChunks) {
						break;
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
					m_ChunksRendered++;
				}
			}
		}

		m_Renderer.CompleteBuffersWrite(m_ChunksRendered);
		m_LastPlayerGridCoords = playerGridCoords;
	}

	void render() {
		m_Renderer.render();
	}

	//void setBlock(const glm::ivec3& coords, uint16_t type);
	//void removeBlock(const glm::ivec3& voxelWorldCoords) {
	//	
	//	const int chunkSize = ChunkType::Size;

	//	glm::ivec3 chunkCoords(
	//		floor(voxelWorldCoords.x / (float)chunkSize),
	//		floor(voxelWorldCoords.y / (float)chunkSize),
	//		floor(voxelWorldCoords.z / (float)chunkSize)
	//	);
	//	//std::cout << "Chunk Coordinates: " << chunkX << ", " << chunkY << ", " << chunkZ << std::endl;

	//	ChunkType* chunk = m_Chunks.getChunk(chunkCoords);

	//	if (!chunk) {
	//		return;
	//	}

	//	size_t xLocal = ((voxelWorldCoords.x % chunkSize) + chunkSize) % chunkSize;
	//	size_t yLocal = ((voxelWorldCoords.y % chunkSize) + chunkSize) % chunkSize;
	//	size_t zLocal = ((voxelWorldCoords.z % chunkSize) + chunkSize) % chunkSize;
	//	//std::cout << "Voxel (local): " << xC << ", " << yC << ", " << zC << std::endl;
	//	chunk->toggleBit(xLocal, yLocal, zLocal);
	//	
	//	// re-mesh chunk
	//	ChunkQuads mesh = m_Mesher.meshChunk(m_Chunks, chunkCoords);
	//	uint64_t encodedChunkCoords = ChunkGrid<ChunkType>::EncodeChunkCoords(chunkCoords);
	//	m_Renderer.UpdateMesh(mesh.quadData, m_ChunkCoordsPageID[encodedChunkCoords]);

	//	// re-mesh neighbooring m_Chunks if voxel remove was on edge of chunk
	//	//if (xLocal == 0) {
	//	//	glm::ivec3 neighboorChunk = chunkCoords + glm::ivec3(-1, 0, 0);
	//	//	ChunkQuads mesh = m_Mesher.meshChunk(m_Chunks, neighboorChunk);
	//	//	uint64_t encodedNeighboorChunkCoords = ChunkGrid<ChunkType>::EncodeChunkCoords(neighboorChunk);
	//	//	m_Renderer.UpdateMesh(mesh.quadData, m_ChunkCoordsPageID[encodedNeighboorChunkCoords]);
	//	//}
	//	//else if (xLocal == chunkSize - 1) {
	//	//	glm::ivec3 neighboorChunk = chunkCoords + glm::ivec3(1, 0, 0);
	//	//	ChunkQuads mesh = m_Mesher.meshChunk(m_Chunks, neighboorChunk);
	//	//	uint64_t encodedNeighboorChunkCoords = ChunkGrid<ChunkType>::EncodeChunkCoords(neighboorChunk);
	//	//	m_Renderer.UpdateMesh(mesh.quadData, m_ChunkCoordsPageID[encodedNeighboorChunkCoords]);
	//	//}
	//	
	//}

	void AddChunk(const glm::ivec3& chunkCoords, const ChunkType& chunk, bool doMesh = false) {
		m_Chunks.addChunk(chunk, chunkCoords);
	}
	void RemoveChunk(const glm::ivec3& chunkCoords);

	void MeshWorld() {
		ChunkQuads mesh;

		for (const auto& [encodedChunkCoords, _] : m_Chunks) {
			glm::ivec3 coords = ChunkGrid<ChunkType>::DecodeChunkCoords(encodedChunkCoords);
			mesh = m_Mesher.meshChunk(m_Chunks, coords);
			const size_t pageId = m_Renderer.UploadMesh(mesh.quadData);
			m_ChunkCoordsPageID[encodedChunkCoords] = pageId;
		}

	}

	void UpdateChunkMesh(const glm::ivec3& coords) {
		// step 1: calculate mesh of chunk
		// step 2: upload new mesh GPU
		// step 3: update indirect draw commands if chunk is being drawn
	}

	void saveModel(const char* filePath);

	inline ChunkGrid<ChunkType>& getGrid() const { return const_cast<ChunkGrid<ChunkType>&>(m_Chunks); };

private:
	ChunkGrid<ChunkType> m_Chunks;
	VoxelMesher<ChunkType> m_Mesher;
	VoxelRenderer m_Renderer;

	unsigned int m_WorldSize; // NxNxN m_Chunks
	int m_RenderDistance;
	unsigned int m_MaxRenderableChunks;
	unsigned int m_ChunksRendered;
	glm::ivec3 m_LastPlayerGridCoords;

	std::unordered_map<uint64_t, size_t> m_ChunkCoordsPageID;

};

typedef World<Chunk8> World8;
typedef World<Chunk16> World16;
typedef World<Chunk32> World32;

#endif

#ifndef VOXEL_RAY_CAST_H
#define VOXEL_RAY_CAST_H

#include <glm/glm.hpp> 
#include <memory>
#include <iostream>


#include "chunk_provider_concept.h"
#include "chunk.h"
#include "voxel_math.h"
#include "logger.h"
#include "profiler.h"


//https://www.cse.yorku.ca/~amana/research/grid.pdf

struct Ray
{
	glm::vec3 origin;
	glm::vec3 direction;
	int voxelSteps;
};

// TODO: optimize _IsVoxelSolid to cache chunk & profile
template <typename ChunkType>
class VoxelRayCast
{
public:
	template <ChunkProvider<ChunkType> ChunkContainer>
	[[nodiscard]] static bool cast(const Ray& ray, const ChunkContainer& chunkContainer, glm::ivec3& hitPos);
private:
	template <ChunkProvider<ChunkType> ChunkContainer>
	inline static bool _IsVoxelSolid(const ChunkContainer& chunkContainer, const glm::ivec3& voxelCoords);
};


typedef VoxelRayCast<Chunk8> VoxelRayCast8;
typedef VoxelRayCast<Chunk16> VoxelRayCast16;
typedef VoxelRayCast<Chunk32> VoxelRayCast32;

#include "voxel_ray_cast.tpp"

#endif


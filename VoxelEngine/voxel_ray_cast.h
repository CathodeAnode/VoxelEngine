#ifndef VOXEL_RAY_CAST_H
#define VOXEL_RAY_CAST_H

#include <glm/glm.hpp>

#include "chunk_grid.h"


//https://www.cse.yorku.ca/~amana/research/grid.pdf

struct Ray
{
	glm::vec3 origin;
	glm::vec3 direction;
	int range;

	Ray(const glm::vec3& _origin, const glm::vec3& _direction, int _range) : 
		origin(_origin), 
		direction(_direction),
		range(_range)
	{}
};

template<typename ChunkType>
class VoxelRayCast
{
public:
	// Optimization: step over empty chunks or null chunks in one step
	// step 1: check if current chunk is empty or null
	// step 2: check how many voxels till next chunk
	// step 3: traverse by number of voxels (tDelta * numOfVoxelsTillNextChunk) & step += numOfVoxelsTillNextChunk
	// step 4: if not out of range check if voxel is solid & repeat loop
	static bool cast(const Ray& ray, const ChunkGrid<ChunkType>& grid, glm::ivec3& hitPos) 
	{
		const int voxelUnit = 1;

		glm::ivec3 currentVoxel(ray.origin);
		glm::vec3 step(
			ray.direction.x > 0 ? voxelUnit : (ray.direction.x < 0 ? -voxelUnit : 0),
			ray.direction.y > 0 ? voxelUnit : (ray.direction.y < 0 ? -voxelUnit : 0),
			ray.direction.z > 0 ? voxelUnit : (ray.direction.z < 0 ? -voxelUnit : 0)
			);

		glm::vec3 tMax = glm::vec3(
			(ray.direction.x > 0 ? currentVoxel.x + 1 : currentVoxel.x) - ray.origin.x,
			(ray.direction.y > 0 ? currentVoxel.y + 1 : currentVoxel.y) - ray.origin.y,
			(ray.direction.z > 0 ? currentVoxel.z + 1 : currentVoxel.z) - ray.origin.z
		) / ray.direction;
		glm::vec3 tDelta = glm::vec3(voxelUnit / glm::abs(ray.direction.x), voxelUnit / glm::abs(ray.direction.y), voxelUnit / glm::abs(ray.direction.z));

		int steps = 0;
		while (steps < ray.range) {
			if (grid.isVoxelSolid(currentVoxel)) {
				hitPos = currentVoxel;
				return true;
			}

			if (tMax.x < tMax.y && tMax.x < tMax.z) {
				// move in x direction
				currentVoxel.x += step.x;
				tMax.x += tDelta.x;
			}
			else if (tMax.y < tMax.x && tMax.y < tMax.z) {
				// move in y direction
				currentVoxel.y += step.y;
				tMax.y += tDelta.y;
			}
			else {
				// move in z direction
				currentVoxel.z += step.z;
				tMax.z += tDelta.z;
			}

			steps++;
		}

		return false;
	}

private:
	//const int chunkSize = ChunkType::Size;
};

typedef VoxelRayCast<Chunk8> VoxelRayCast8;
typedef VoxelRayCast<Chunk16> VoxelRayCast16;
typedef VoxelRayCast<Chunk32> VoxelRayCast32;

#endif


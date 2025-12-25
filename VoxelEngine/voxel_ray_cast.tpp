#ifndef VOXEL_RAY_CAST_TPP
#define VOXEL_RAY_CAST_TPP

#include "voxel_ray_cast.h"


template<typename ChunkType>
template<ChunkProvider<ChunkType> ChunkContainer>
inline bool VoxelRayCast<ChunkType>::cast(const Ray& ray, const ChunkContainer& chunkContainer, glm::ivec3& hitPos)
{
	const int voxelUnit = 1;
	glm::ivec3 currentVoxel(glm::floor(ray.origin));
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
	while (steps < ray.voxelSteps) {
		if (_IsVoxelSolid(chunkContainer, currentVoxel)) {
			hitPos = currentVoxel;
			return true;
		}

		if (tMax.x <= tMax.y && tMax.x <= tMax.z) {
			// move in x direction
			currentVoxel.x += step.x;
			tMax.x += tDelta.x;
		}
		else if (tMax.y <= tMax.x && tMax.y <= tMax.z) {
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

template<typename ChunkType>
template<ChunkProvider<ChunkType> ChunkContainer>
inline bool VoxelRayCast<ChunkType>::_IsVoxelSolid(const ChunkContainer& chunkContainer, const glm::ivec3& voxelCoords)
{
	const int chunkSize = ChunkType::Size;

	const glm::ivec3 chunkCoords = WorldToChunk(voxelCoords, chunkSize);

	std::shared_ptr<const ChunkType> chunk = chunkContainer.GetChunk(chunkCoords);

	const glm::ivec3 localVoxelCoords = VoxelToLocal(voxelCoords, chunkSize);

	return chunk->IsSolid(localVoxelCoords.x, localVoxelCoords.y, localVoxelCoords.z);
}

#endif
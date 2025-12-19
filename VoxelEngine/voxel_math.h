#ifndef VOXEL_MATH_H
#define VOXEL_MATH_H

#include <glm/glm.hpp>

/// <summary>
/// mod that returns [0, divisor)
/// </summary>
/// <param name="val">Value</param>
/// <param name="div">Divisor</param>
/// <returns></returns>
inline int PositiveMod(int val, int div)
{
	return ((val % div) + div) % div;
}

/// <summary>
/// Convert world voxel coords to local chunk coords
/// </summary>
/// <param name="worldPos"></param>
/// <param name="chunkSize"></param>
/// <returns></returns>
inline glm::ivec3 WorldToChunk(const glm::ivec3& worldPos, int chunkSize)
{
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x / chunkSize)),
        static_cast<int>(std::floor(worldPos.y / chunkSize)),
        static_cast<int>(std::floor(worldPos.z / chunkSize))
    );
}

/// <summary>
/// Convert world voxel coords to local voxel coords inside chunk
/// </summary>
/// <param name="voxelCoords"></param>
/// <param name="chunkSize"></param>
/// <returns></returns>
inline glm::ivec3 VoxelToLocal(const glm::ivec3& voxelCoords, int chunkSize)
{
    return glm::ivec3(
        PositiveMod(voxelCoords.x, chunkSize),
        PositiveMod(voxelCoords.y, chunkSize),
        PositiveMod(voxelCoords.z, chunkSize)
    );
}

#endif 


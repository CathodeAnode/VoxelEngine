#ifndef VOXEL_MATH_H
#define VOXEL_MATH_H

#include <glm/glm.hpp>
#include <cmath>

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
/// Integer floor division (works correctly for negative numbers)
/// </summary>
inline int FloorDiv(int val, int div) 
{
    int q = val / div;
    int r = val % div;

    // if val < 0 and r != 0, subtract 1
    return q - ((r != 0) & (val < 0));
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
        FloorDiv(worldPos.x, chunkSize),
        FloorDiv(worldPos.y, chunkSize),
        FloorDiv(worldPos.z, chunkSize)
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

inline size_t NextPowerOfTwo(size_t n)
{
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    if constexpr (sizeof(size_t) == 8)
        n |= n >> 32;
    return n + 1;
}

#endif 


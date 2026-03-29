#include "uid_manager.h"
#include <glm/vec3.hpp>
#include <cassert>


VoxelObjectID UIDManager::Generate()
{
    VoxelObjectID id = ++s_EntityCounter;

    // Ensure we don't overflow into MSB region accidentally
    assert(id < k_TypeBitMask);

    return k_TypeBitMask | id;
}

VoxelObjectID UIDManager::Generate(glm::ivec3 chunkCoord)
{
    assert(_IsValidCoord(chunkCoord.x));
    assert(_IsValidCoord(chunkCoord.y));
    assert(_IsValidCoord(chunkCoord.z));

    VoxelObjectID ux = _EncodeCoord(chunkCoord.x);
    VoxelObjectID uy = _EncodeCoord(chunkCoord.y);
    VoxelObjectID uz = _EncodeCoord(chunkCoord.z);

    return (ux << (k_CoordBits * 2)) |
        (uy << (k_CoordBits)) |
        (uz);
}

bool UIDManager::IsEntity(VoxelObjectID id)
{
    return (id & k_TypeBitMask) != 0;
}

bool UIDManager::IsChunk(VoxelObjectID id)
{
    return (id & k_TypeBitMask) == 0;
}

glm::ivec3 UIDManager::DecodeChunk(VoxelObjectID id)
{
    assert(IsChunk(id));

    VoxelObjectID uz = id & k_CoordMask;
    VoxelObjectID uy = (id >> k_CoordBits) & k_CoordMask;
    VoxelObjectID ux = (id >> (k_CoordBits * 2)) & k_CoordMask;

    return glm::ivec3(_DecodeCoord(ux), _DecodeCoord(uy), _DecodeCoord(uz));
}

bool UIDManager::_IsValidCoord(int64_t v)
{
    return v >= k_MinCoord && v <= k_MaxCoord;
}

VoxelObjectID UIDManager::_EncodeCoord(int64_t v)
{
    return VoxelObjectID(v) & k_CoordMask;
}

int64_t UIDManager::_DecodeCoord(VoxelObjectID v)
{
    const VoxelObjectID signBit =
        VoxelObjectID(1) << (k_CoordBits - 1);

    if (v & signBit)
        return int64_t(v | (~k_CoordMask)); // sign extend
    else
        return int64_t(v);
}

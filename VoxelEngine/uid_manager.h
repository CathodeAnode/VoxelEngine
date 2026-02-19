#ifndef UID_MANAGER_H
#define UID_MANAGER_H

#include <atomic>
#include <glm/fwd.hpp>
#include "types.h"


#include <atomic>
#include <cstdint>
#include <cassert>


class UIDManager
{
public:
    static VoxelObjectID Generate()
    {
        VoxelObjectID id = ++s_EntityCounter;

        // Ensure we don't overflow into MSB region accidentally
        assert(id < k_TypeBitMask);

        return k_TypeBitMask | id;
    }

    static VoxelObjectID Generate(glm::ivec3 chunkCoord)
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

    static bool IsEntity(VoxelObjectID id)
    {
        return (id & k_TypeBitMask) != 0;
    }

    static bool IsChunk(VoxelObjectID id)
    {
        return (id & k_TypeBitMask) == 0;
    }

    static glm::ivec3 DecodeChunk(VoxelObjectID id)
    {
        assert(IsChunk(id));

        VoxelObjectID uz = id & k_CoordMask;
        VoxelObjectID uy = (id >> k_CoordBits) & k_CoordMask;
        VoxelObjectID ux = (id >> (k_CoordBits * 2)) & k_CoordMask;

        return glm::ivec3( _DecodeCoord(ux), _DecodeCoord(uy), _DecodeCoord(uz));
    }

private:

    // Layout constants
    static constexpr int k_TotalBits = sizeof(VoxelObjectID) * 8;
    static constexpr int k_TypeBits = 1; // Mask for MSB (entity flag)
    static constexpr int k_CoordBits = (k_TotalBits - k_TypeBits) / 3; // 21 for 64bit

    static constexpr VoxelObjectID k_TypeBitMask = VoxelObjectID(1) << (k_TotalBits - 1);

    static constexpr VoxelObjectID k_CoordMask = (VoxelObjectID(1) << k_CoordBits) - 1;

    static constexpr int64_t k_MinCoord = -(int64_t(1) << (k_CoordBits - 1));
    static constexpr int64_t k_MaxCoord = (int64_t(1) << (k_CoordBits - 1)) - 1;

    static bool _IsValidCoord(int64_t v)
    {
        return v >= k_MinCoord && v <= k_MaxCoord;
    }

    // Convert signed coord to unsigned packed representation
    static VoxelObjectID _EncodeCoord(int64_t v)
    {
        return VoxelObjectID(v) & k_CoordMask;
    }

    static int64_t _DecodeCoord(VoxelObjectID v)
    {
        // sign extend manually
        const VoxelObjectID signBit = VoxelObjectID(1) << (k_CoordBits - 1);

        if (v & signBit)
            return int64_t(v | (~k_CoordMask)); // extend sign
        else
            return int64_t(v);
    }

    static_assert(std::is_unsigned_v<VoxelObjectID>, "VoxelObjectID must be unsigned");
    static_assert(k_TotalBits >= 64, "VoxelObjectID too small");
    static_assert((k_TotalBits - k_TypeBits) % 3 == 0, "Bit layout must divide evenly across 3 coords");

private:
    static inline std::atomic<VoxelObjectID> s_EntityCounter{ 0 };
};


#endif


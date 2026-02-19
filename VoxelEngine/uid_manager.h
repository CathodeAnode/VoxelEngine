#ifndef UID_MANAGER_H
#define UID_MANAGER_H

#include <atomic>
#include <glm/fwd.hpp>
#include "types.h"


#include <atomic>
#include <cstdint>
#include <cassert>

#define NULL_VOXELOBJECTID 

/**
 * @brief Generates and manages unique identifiers for entities and chunks.
 *
 * IDs use a structured bit layout to make them self-describing.
 *
 * Layout (64-bit architecture example):
 * - 1 most significant bit (type flag)
 *     - 1 => Entity ID
 *     - 0 => Chunk ID
 * - Remaining bits are evenly divided across X, Y, Z chunk coordinates.
 *
 * Example layout:
 * @code
 * [ MSB |     X     |     Y     |     Z     ]
 * [ 1b  |  N bits   |  N bits   |  N bits   ]
 * @endcode
 *
 * Coordinate bit count depends on the size of VoxelObjectID.
 */
class UIDManager
{
public:
    /// Null / invalid ID (MSB set, rest 0)
    static constexpr VoxelObjectID NullVoxelObjectID = VoxelObjectID(1) << (sizeof(VoxelObjectID) * 8 - 1);

    /**
     * @brief Generate a unique entity ID.
     * @return Entity ID with type bit set.
     */
    static VoxelObjectID Generate();

    /**
     * @brief Generate a chunk ID from 3D coordinates.
     * @param chunkCoord Chunk grid coordinate.
     * @return Packed chunk ID (type bit cleared).
     */
    static VoxelObjectID Generate(glm::ivec3 chunkCoord);

    /**
     * @brief Check if ID represents an entity.
     */
    static bool IsEntity(VoxelObjectID id);

    /**
     * @brief Check if ID represents a chunk.
     */
    static bool IsChunk(VoxelObjectID id);

    /**
     * @brief Decode a chunk ID into its 3D coordinate.
     * @param id Chunk ID.
     * @return Decoded chunk coordinate.
     */
    static glm::ivec3 DecodeChunk(VoxelObjectID id);

private:

    // Layout constants

    /// Total number of bits in VoxelObjectID.
    static constexpr int k_TotalBits = sizeof(VoxelObjectID) * 8;

    /// Mask for MSB (entity flag)
    static constexpr int k_TypeBits = 1; 

    /// Mask for one coordinate
    static constexpr int k_CoordBits = (k_TotalBits - k_TypeBits) / 3; 

    /// Mask for type bit (entity flag).
    static constexpr VoxelObjectID k_TypeBitMask = VoxelObjectID(1) << (k_TotalBits - 1); 

    /// Mask for extracting a single coordinate
    static constexpr VoxelObjectID k_CoordMask = (VoxelObjectID(1) << k_CoordBits) - 1;

    /// Minimum allowed coordinate value, based on VoxelObjectID uint type
    static constexpr int64_t k_MinCoord = -(int64_t(1) << (k_CoordBits - 1));

    /// Maxiumium allowed coordinate value, based on VoxelObjectID uint type
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


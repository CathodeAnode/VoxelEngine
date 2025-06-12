#ifndef VOXEL_GEOMETRY_H
#define VOXEL_GEOMETRY_H

#include <array>
#include <vector>

#include "types.h"

// Enum representing the six possible faces of a voxel
enum class VoxelFace {
    Up,     // +Y
    Down,   // -Y
    Left,   // -X
    Right,  // +X
    Front,  // +Z
    Back    // -Z
};

// VoxelGeometry provides vertex data for voxel faces.
// Each face consists of 6 vertices (2 triangles) with 4 floats per vertex.
// Format: [x, y, z, attr] * 6 = 24 floats
class VoxelGeometry {
public:
    // Get the vertex data for a given voxel face.
    // The returned array has 24 floats (6 vertices × 4 floats each).
    static const std::array<float, 24>& getFaceVertices(VoxelFace face);

    // Number of vertices per face (6: 2 triangles)
    static int getFaceVertexCount() { return 6; }

    // Size in bytes of one vertex (4 floats)
    static size_t getVertexStride() { return 4 * sizeof(float); }

    // Vertex layout: position (vec3), attribute (float)
    static const std::vector<VertexAttribute>& getVertexLayout();
};

#endif // VOXEL_GEOMETRY_H


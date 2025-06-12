#include "voxel_geometry.h"

const std::array<float, 24>& VoxelGeometry::getFaceVertices(VoxelFace face) {
    // Each face: 2 triangles (6 vertices), each vertex: x, y, z, attr
    static const std::array<float, 24> faceVertices[] = {
        // Up (+Y)
        {
            -0.5f, 0.5f, -0.5f, 0,   0.5f, 0.5f, -0.5f, 1,   0.5f, 0.5f,  0.5f, 1,
            -0.5f, 0.5f, -0.5f, 0,   0.5f, 0.5f,  0.5f, 1,  -0.5f, 0.5f,  0.5f, 0
        },
        // Down (-Y)
        {
            -0.5f, -0.5f, -0.5f, 0,   0.5f, -0.5f,  0.5f, 1,   0.5f, -0.5f, -0.5f, 1,
            -0.5f, -0.5f, -0.5f, 0,  -0.5f, -0.5f,  0.5f, 0,   0.5f, -0.5f,  0.5f, 1
        },
        // Left (-X)
        {
            -0.5f, -0.5f, -0.5f, 0,  -0.5f,  0.5f,  0.5f, 1,  -0.5f,  0.5f, -0.5f, 1,
            -0.5f, -0.5f, -0.5f, 0,  -0.5f, -0.5f,  0.5f, 0,  -0.5f,  0.5f,  0.5f, 1
        },
        // Right (+X)
        {
             0.5f, -0.5f, -0.5f, 0,   0.5f,  0.5f, -0.5f, 1,   0.5f,  0.5f,  0.5f, 1,
             0.5f, -0.5f, -0.5f, 0,   0.5f,  0.5f,  0.5f, 1,   0.5f, -0.5f,  0.5f, 0
        },
        // Front (+Z)
        {
            -0.5f, -0.5f, 0.5f, 0,   0.5f,  0.5f, 0.5f, 1,  -0.5f,  0.5f, 0.5f, 0,
            -0.5f, -0.5f, 0.5f, 0,   0.5f, -0.5f, 0.5f, 1,   0.5f,  0.5f, 0.5f, 1
        },
        // Back (-Z)
        {
            -0.5f, -0.5f, -0.5f, 0,  -0.5f,  0.5f, -0.5f, 0,   0.5f,  0.5f, -0.5f, 1,
            -0.5f, -0.5f, -0.5f, 0,   0.5f,  0.5f, -0.5f, 1,   0.5f, -0.5f, -0.5f, 1
        }
    };

    return faceVertices[static_cast<int>(face)];
}

const std::vector<VertexAttribute>& VoxelGeometry::getVertexLayout() {
    static const std::vector<VertexAttribute> layout = {
        {0, 3, GL_FLOAT, GL_FALSE, 0},                  // vec3 position at offset 0
        {1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float)}   // 1 float attribute at offset 12
    };
    return layout;
}
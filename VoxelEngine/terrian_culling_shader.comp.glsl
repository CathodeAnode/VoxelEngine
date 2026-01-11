#version 460


// 256 threads => 8 wraps
layout(local_size_x = 8, local_size_y = 8, local_size_z = 4) in;

//struct Frustum
//{
//	glm::vec4 leftClipPlane;
//	glm::vec4 rightClipPlane;
//	glm::vec4 bottomClipPlane;
//	glm::vec4 topClipPlane;
//	glm::vec4 nearClipPlane;
//	glm::vec4 farClipPlane;
//};

struct ChunkAABB
{
    ivec3 min;
    ivec3 max;
};

// --------------------
// Inputs (Uniforms)
// --------------------
uniform vec4 u_FrustumPlanes[6];
uniform uint u_ChunkSize;
uniform ivec3 u_CameraPos;


// --------------------
// Output SSBO
// --------------------
layout(std430, binding = 1) buffer ResultBuffer
{
    uint  count;
    ivec4 results[];
};


bool test_AABB_against_frustum(ChunkAABB aabb)
{
    for (int i = 0; i < 6; ++i)
    {
        vec3 n = u_FrustumPlanes[i].xyz;
        float d = u_FrustumPlanes[i].w;

        vec3 p;
        p.x = (n.x >= 0.0) ? aabb.max.x : aabb.min.x;
        p.y = (n.y >= 0.0) ? aabb.max.y : aabb.min.y;
        p.z = (n.z >= 0.0) ? aabb.max.z : aabb.min.z;

        if (dot(n, p) + d < 0.0)
            return false;
    }
    return true;
}

// --------------------
// Main
// --------------------
void main()
{
    //TODO: use shared mem count to write results in batches
    //TODO: gpu indirect draw 

    ivec3 chunkCoord = ivec3(gl_GlobalInvocationID) + u_CameraPos;

    // Build AABB
    ChunkAABB chunkAABB;
    chunkAABB.min = chunkCoord;
    chunkAABB.max = chunkAABB.min + ivec3(u_ChunkSize);

    // Frustum culling
    if (test_AABB_against_frustum(chunkAABB))
    {
        // Atomically get write index
        uint writeIndex = atomicAdd(count, 1);

        // Append visible chunk coordinate
        results[writeIndex] = ivec4(chunkCoord, 0);;
    }

}

#version 460
#extension GL_ARB_gpu_shader_int64 : require

// TODOs:
// - Implement #define preprocessor (SGL)
// - Implement const injection from cpu side (SGL)
// - Use SGL instead of shader.h

const uint NULL_PAGE = 0xFFFFFFFFU;
const uint CACHE_PAGE_SIZE = 400;
const uint64_t EMPTY_KEY = 0xFFFFFFFFFFFFFFFFUL;
const uint REF_BIT = 2; // 0b10

struct ChunkAABB
{
    ivec3 min;
    ivec3 max;
};

struct ClockPolicyHandle
{
    uint64_t objID;
};

struct ObjectAllocation
{
    uint totalElementCount;
    uint startPage;
    uint endPage;
    ClockPolicyHandle policyHandle;
};

struct MapEntry
{
    uint64_t key;
    ObjectAllocation value;
};

struct PageNode
{
    uint next;
};

struct DrawArraysIndirectCommand 
{
    uint count;
    uint instanceCount;
    uint first;
    uint baseInstance;
};

layout(local_size_x = 8, local_size_y = 4, local_size_z = 8) in; // 256 threads => 8 wraps


layout(std430, binding = 0) buffer IndirectDrawBuffer
{
    uint indirectCmdsCount;
    uint pad_indirect[3];  

    DrawArraysIndirectCommand indirectCmds[];
};

layout(std430, binding = 1) buffer chunkPosBuffer
{
    vec4 chunkPositions[];
};

layout(std430, binding = 2) buffer UncachedChunks
{
    uint  count;
    uint pad_uncached[3];  

    ivec4 results[];
};

layout(std430, binding = 3) readonly buffer CacheObjTable
{
    MapEntry objectData[];
};

layout(std430, binding = 4) readonly buffer PageNodesBuffer
{
    PageNode pageNodes[];
};


uniform vec4 u_FrustumPlanes[6];
uniform uint u_RenderDistance;
uniform uint u_ChunkSize;
uniform ivec3 u_CameraChunkPos;

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

uint64_t Hash64(uint64_t x)
{
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdUL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53UL;
    x ^= x >> 33;
    return x;
}

uint MapIndex(uint64_t obj)
{
    uint64_t h = Hash64(obj);
    return uint(h & (objectData.length() - 1));
}

bool IsCached(uint64_t key, out ObjectAllocation alloc)
{
    uint cap = objectData.length();
    uint start = MapIndex(key);

    for (uint probe = 0u; probe < cap; ++probe)
    {
        uint idx = (start + probe) % cap;
        MapEntry entry = objectData[idx];

        if (entry.key == EMPTY_KEY)
        {
            alloc = entry.value;
            return false;
        }

        if (entry.key == key)
        {
            alloc = entry.value;
            return true;
        }
    }

    return false;
}

void DrawChunk(ObjectAllocation alloc, ivec3 chunkPos)
{
    uint current = alloc.startPage;
    uint start = alloc.startPage;
    
    while (current != NULL_PAGE)
    {
        uint next = pageNodes[current].next;

        if(next != current + 1)
        {
            uint index = atomicAdd(indirectCmdsCount, 1);

            // range is from start -> current (write to indirect buffer as DrawArraysIndirectCommand)
            indirectCmds[index].count = 4;
            indirectCmds[index].first = 0;
            indirectCmds[index].baseInstance = start * CACHE_PAGE_SIZE;
            indirectCmds[index].instanceCount = (current - start) * CACHE_PAGE_SIZE;

            indirectCmds[index].instanceCount += uint(current == alloc.endPage) * alloc.totalElementCount; // Branchless addition
            chunkPositions[index] = vec4(chunkPos, 0) * u_ChunkSize;

            start = next;
        }

        current = next;
    }
}

void main()
{
    //TODO: use shared mem count to write results in batches

    ivec3 dim = ivec3(u_RenderDistance);

    // outside render distance
    ivec3 offset = ivec3(gl_GlobalInvocationID) - dim;
    if (any(greaterThan(ivec3(offset), dim)))
        return; // skip out-of-bounds threads

    ivec3 chunkCoord = u_CameraChunkPos + offset;

    // Build AABB
    ChunkAABB chunkAABB;
    chunkAABB.min = chunkCoord * int(u_ChunkSize);
    chunkAABB.max = chunkAABB.min + ivec3(u_ChunkSize);

    // Frustum culling
    if (test_AABB_against_frustum(chunkAABB))
    {
        uint64_t chunkID =
            (uint64_t(0) << 63) | // chunk = 0
            (uint64_t(chunkCoord.x & 0x1FFFFF) << 42) |
            (uint64_t(chunkCoord.y & 0x1FFFFF) << 21) |
            uint64_t(chunkCoord.z & 0x1FFFFF);

        ObjectAllocation alloc;
        if(IsCached(chunkID, alloc))
        {
            DrawChunk(alloc, chunkCoord);
        }
        else
        {
            uint index = atomicAdd(count, 1);
            results[index] = ivec4(chunkCoord, 0);
        }
    }
}

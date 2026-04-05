#version 460

// TODOs:
// - Implement #define preprocessor (SGL)
// - Implement const injection from cpu side (SGL)
// - Use SGL instead of shader.h

const uint NULL_PAGE = 65535;
const uint CACHE_PAGE_SIZE = 200;
const uvec2 EMPTY_KEY = uvec2(0xFFFFFFFFu, 0xFFFFFFFFu);
const uvec2 C1 = uvec2(0xed558ccdU, 0xff51afd7U);
const uvec2 C2 = uvec2(0x1a85ec53U, 0xc4ceb9feU);
const uint REF_BIT = 2; // 0b10

struct ChunkAABB
{
    ivec3 min;
    ivec3 max;
};

struct ClockPolicyHandle
{
    uint index;
};

struct ObjectAllocation
{
    uint totalElementCount;
    uint startPage; // uint16
    uint endPage; // uint16
    ClockPolicyHandle policyHandle;
};

struct MapEntry
{
    uvec2 key;
    ObjectAllocation value;
};

struct PageNode
{
    uint next; // uint16
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
    DrawArraysIndirectCommand indirectCmds[];
};

layout(std430, binding = 1) buffer UncachedChunks
{
    uint  count;
    ivec4 results[];
};

layout(std430, binding = 2) readonly buffer CacheObjTable
{
    MapEntry objectData[];
};

layout(std430, binding = 3) readonly buffer PageNodesBuffer
{
    PageNode pageNodes[];
};

layout(std430, binding = 4) buffer ClockPolicyState
{
    uint policyObjectState[];
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

uvec2 GetChunkID(ivec3 chunkCoords)
{
    const uint typeFlag = 0u; // Type flag = 0 for Chunk ID
    const uint mask21 = 0x1FFFFFu; // 21-bit mask

    uint x = uint(chunkCoords.x) & mask21;
    uint y = uint(chunkCoords.y) & mask21;
    uint z = uint(chunkCoords.z) & mask21;

    uvec2 result;

    // Pack lower 32 bits (uvec2.x)
    // Bits 0–20: Z
    // Bits 21–31: lower 11 bits of Y
    result.x = (y & 0x7FFu) << 21 | (z & 0x1FFFFFu);

    // Pack upper 32 bits (uvec2.y)
    // Bits 0–9: upper 10 bits of Y
    // Bits 10–30: X
    // Bit 31: type flag
    result.y = (x << 10) | ((y >> 11) & 0x3FFu) | (typeFlag << 31);

    return result;
}

// 64-bit right shift
uvec2 shr64(uvec2 v, uint s) 
{
    if (s == 0u) return v;
    if (s < 32u) 
    {
        return uvec2(
            (v.x >> s) | (v.y << (32u - s)),
            v.y >> s
        );
    } 
    else 
    {
        return uvec2(
            v.y >> (s - 32u),
            0u
        );
    }
}

// 64-bit multiply
uvec2 mul64(uvec2 a, uvec2 b) 
{
    uint lo = a.x * b.x;

    uint mid1 = a.x * b.y;
    uint mid2 = a.y * b.x;

    uint hi = a.y * b.y;

    uint carry = ((lo >> 16u) + (mid1 & 0xFFFFu) + (mid2 & 0xFFFFu)) >> 16u;

    hi += (mid1 >> 16u) + (mid2 >> 16u) + carry;

    return uvec2(lo, hi);
}

// XOR
uvec2 xor64(uvec2 a, uvec2 b) 
{
    return uvec2(a.x ^ b.x, a.y ^ b.y);
}

uint Hash64(uvec2 x)
{
    x = xor64(x, shr64(x, 33u));
    x = mul64(x, C1);

    x = xor64(x, shr64(x, 33u));
    x = mul64(x, C2);

    x = xor64(x, shr64(x, 33u));

    return x.x; // lower 32 bits
}

uint MapIndex(uvec2 obj)
{
    uint h = Hash64(obj);
    return h & (objectData.length() - 1);
}

bool IsCached(uvec2 obj, out ObjectAllocation alloc)
{
    uint cap = objectData.length();
    uint start = MapIndex(obj);

    for (uint probe = 0u; probe < cap; ++probe)
    {
        uint idx = (start + probe) & (cap - 1u);
        MapEntry entry = objectData[idx];

        uvec2 current = entry.key;

        if (all(equal(current, EMPTY_KEY)))
        {
            alloc = entry.value;
            return false;
        }

        if (all(equal(current, obj)))
        {
            alloc = entry.value;
            return true;
        }
    }

    return false;
}

void DrawChunk(ObjectAllocation alloc)
{
    uint current = alloc.startPage;
    uint start = alloc.startPage;
    
    while (current != NULL_PAGE)
    {
        uint next = pageNodes[current].next;

        if(next != start + 1)
        {
            uint index = atomicAdd(indirectCmdsCount, 1);

            // range is from start -> current (write to indirect buffer as DrawArraysIndirectCommand)
            indirectCmds[index].count = 4;
            indirectCmds[index].first = 0;
            indirectCmds[index].baseInstance = start * CACHE_PAGE_SIZE;
            indirectCmds[index].instanceCount = (current - start) * CACHE_PAGE_SIZE;

            indirectCmds[index].instanceCount += uint(current == alloc.endPage) * alloc.totalElementCount; // Branchless addition

            start = next;
        }

        current = next;
    }

    if (current != start) 
    {
        uint index = atomicAdd(indirectCmdsCount, 1);
        indirectCmds[index].count = 4;
        indirectCmds[index].first = 0;
        indirectCmds[index].baseInstance = start * CACHE_PAGE_SIZE;
        indirectCmds[index].instanceCount = (current - start + 1) * CACHE_PAGE_SIZE + alloc.totalElementCount;
    }
}

void PolicyTouch(ObjectAllocation alloc)
{
    atomicOr(policyObjectState[alloc.policyHandle.index], REF_BIT);
}

void main()
{
    //TODO: use shared mem count to write results in batches

    ivec3 offset = ivec3(gl_GlobalInvocationID) - ivec3(u_RenderDistance);
    ivec3 chunkCoord = u_CameraChunkPos + offset;

    // Build AABB
    ChunkAABB chunkAABB;
    chunkAABB.min = chunkCoord * int(u_ChunkSize);
    chunkAABB.max = chunkAABB.min + ivec3(u_ChunkSize);

    // Frustum culling
    if (test_AABB_against_frustum(chunkAABB))
    {
        uvec2 chunkID = GetChunkID(chunkCoord);
        ObjectAllocation alloc;
        if(IsCached(chunkID, alloc))
        {
            DrawChunk(alloc);
            PolicyTouch(alloc);
        }
        else
        {
            uint index = atomicAdd(count, 1);
            results[index] = ivec4(chunkCoord, 0);
        }
    }
}

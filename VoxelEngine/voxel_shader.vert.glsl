#version 460 core

layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aTextCoord;
layout (location=2) in uint aData;
layout (location=3) in uint aColor;

out vec2 TextCoord;
flat out uint face;
flat out uint color;

uniform mat4 view;
uniform mat4 projection;

layout(std430, binding = 0) readonly buffer ChunkBuffer 
{
    vec4 positions[]; 
};

struct VoxelQuad 
{
    uint x;
    uint y;
    uint z;
    uint width;
    uint height;
    uint direction;
    uint reserved;
};

VoxelQuad unpackVoxelQuad(uint data) 
{
    VoxelQuad quad;

    quad.x    = (data >>  0) & 0x1Fu; // 0b00011111
    quad.y    = (data >>  5) & 0x1Fu;
    quad.z    = (data >> 10) & 0x1Fu;
    quad.width     = (data >> 15) & 0x1Fu;
    quad.height    = (data >> 20) & 0x1Fu;
    quad.direction = (data >> 25) & 0x7u;  // 0b00000111
    quad.reserved  = (data >> 28) & 0xFu;  // 0b00001111

    return quad;
}

void main() 
{
    VoxelQuad q = unpackVoxelQuad(aData);

    float u = aPos.x;
    float v = aPos.y;

    vec3 origin;
    vec3 right;
    vec3 up;

    switch(q.direction)
    {
        case 0u: // +Z
            origin = vec3(q.x, q.y, q.z);
            right  = vec3(1.0, 0.0, 0.0);
            up     = vec3(0.0, 1.0, 0.0);
            right *= float(q.width);
            up    *= float(q.height);
            break;

        case 1u: // -Z
            origin = vec3(q.x + q.width, q.y, q.z + 1);
            right  = vec3(-1.0, 0.0, 0.0);
            up     = vec3(0.0, 1.0, 0.0);
            right *= float(q.width);
            up    *= float(q.height);
            break;

        case 2u: // +X
            origin = vec3(q.x + 1, q.y, q.z);
            right  = vec3(0,0,1);
            up     = vec3(0,1,0);
            right *= float(q.width);
            up    *= float(q.height);
            break;

        case 3u: // -X
            origin = vec3(q.x, q.y, q.z + q.width);
            right  = vec3(0,0,-1);
            up     = vec3(0,1,0);
            right *= float(q.width);
            up    *= float(q.height);
            break;

        case 4u: // +Y
            origin = vec3(q.x, q.y + 1, q.z);
            right  = vec3(1.0, 0.0, 0.0);
            up     = vec3(0.0, 0.0, 1.0);
            right *= float(q.width);
            up    *= float(q.height);
            break;

        default: // 5u (-Y)
            origin = vec3(q.x, q.y, q.z + q.height);
            right  = vec3(1.0, 0.0, 0.0);
            up     = vec3(0.0, 0.0, -1.0);
            right *= float(q.width);
            up    *= float(q.height);
            break;
    }

    vec3 quadPos = origin + right * u + up * v;
    
    
    quadPos += positions[gl_DrawID].xyz;

	gl_Position = projection * view * vec4(quadPos, 1.0f);
    face = q.direction;
    color = aColor;
}
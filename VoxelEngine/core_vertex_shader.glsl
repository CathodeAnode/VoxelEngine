#version 460 core

layout (location=0) in vec3 aPos;
layout (location=1) in vec2 aTextCoord;
layout (location=2) in uint aData;
layout (location=3) in uint aType;

out vec2 TextCoord;
flat out uint face;
//out uint Type;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout(std430, binding = 0) readonly buffer ChunkBuffer {
    vec4 positions[]; 
};

struct VoxelQuad {
    uint x;
    uint y;
    uint z;
    uint width;
    uint height;
    uint direction;
    uint reserved;
};

VoxelQuad unpackVoxelQuad(uint data) {
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

void main() {
    VoxelQuad q = unpackVoxelQuad(aData);
    vec3 quadPos = aPos;
  

    if(q.direction == 1u) {
        // -z face
        quadPos.xy *= vec2(q.width, q.height);
        quadPos += vec3(q.x,q.y,q.z);
    }
    else if(q.direction == 0u) {
        // +z face
        quadPos = quadPos - vec3(0.0f, 0.0, 1.0f);
        quadPos.xy *= vec2(q.width, q.height);
        quadPos += vec3(q.x,q.y,q.z);
    }
    else if(q.direction == 3u) {
        // -x face
        quadPos = quadPos.zyx;
        quadPos.yz *= vec2(q.height, q.width);
        quadPos += vec3(q.x,q.y,q.z);

    }
    else if(q.direction == 2u) {
        // +x face
        quadPos = quadPos.zyx;
        quadPos.yz *= vec2(q.height, q.width);
        quadPos += vec3(q.x,q.y,q.z);
        quadPos.x += 1.0f;
    }
    else if(q.direction == 4u) {
        // +y face
        quadPos = quadPos.xzy;
        quadPos.xz *= vec2(q.width, q.height);
        quadPos += vec3(q.x,q.y,q.z);
        quadPos.y += 1.0f;
    }
    else if(q.direction == 5u) {
        // -y face
        quadPos = quadPos.xzy;
        quadPos.xz *= vec2(q.width, q.height);
        quadPos += vec3(q.x,q.y,q.z);
    }

    
    quadPos += positions[gl_DrawID].xyz;


	gl_Position = projection * view * vec4(quadPos, 1.0f);
    face = q.direction;
	//TextCoord = aTextCoord;
    //Type = aType;
}
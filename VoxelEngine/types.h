#ifndef TYPES_H
#define TYPES_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>

#include <vector>

using QuadMeshData = uint32_t; //TODO: use bitfields struct instead
using RGBAColor = uint32_t;
using VoxelObjectID = uint64_t;


struct VoxelQuad
{
	QuadMeshData data;
	RGBAColor color;
};

struct DrawArraysIndirectCommand
{
	unsigned int count;
	unsigned int instanceCount;
	unsigned int first;
	unsigned int baseInstance;
};

/**
 * Encodes all voxel quad data into a 32-bit unsigned integer (uint32_t, from LSB to MSB),
 * stored in a vector 
 * Bits  0 -  4  (5 bits): Chunk X-coordinate (0–31)
 * Bits  5 -  9  (5 bits): Chunk Y-coordinate (0–31)
 * Bits 10 - 14  (5 bits): Chunk Z-coordinate (0–31)
 * Bits 15 - 19  (5 bits): Width of quad (0–31 units)
 * Bits 20 - 24  (5 bits): Height of quad (0–31 units)
 * Bits 25 - 27  (3 bits): Direction (up, down, left, front, etc.)
 * Bits 28 - 31 (4 bits): Reserved
 */
struct ChunkQuads {
	std::vector<QuadMeshData> chunkQuads;
	std::vector<RGBAColor> quadColors;

	void AddRawQuad(QuadMeshData quadData, RGBAColor quadColor)
	{
		chunkQuads.push_back(quadData);
		quadColors.push_back(quadColor);
	}

	void AddQuad(int x, int y, int z, int w, int h, int dir, RGBAColor quadColor) {
		//std::cout << "Face: " << dir << " pos: (" << x << "," << y << "," << z << ") "
		//	<< "size: (" << w << "x" << h << ")\n";
		QuadMeshData quadVal = 0;
		quadVal |= (x & 0x1F) << 0;
		quadVal |= (y & 0x1F) << 5;
		quadVal |= (z & 0x1F) << 10;
		quadVal |= (w & 0x1F) << 15;
		quadVal |= (h & 0x1F) << 20;
		quadVal |= (dir & 0x07) << 25;
		
		chunkQuads.push_back(quadVal);
		quadColors.push_back(quadColor);
  	}
};


static_assert(std::is_integral<VoxelObjectID>::value, "VoxelObjectID must be an integral type");

#endif

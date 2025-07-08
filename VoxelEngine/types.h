#ifndef TYPES_H
#define TYPES_H

#include <glad/glad.h>
#include <iostream>

#include <vector>

enum class QuadFaceDir {
	Up,     // +Y
	Down,   // -Y
	Left,   // -X
	Right,  // +X
	Front,  // +Z
	Back    // -Z
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
	std::vector<uint32_t> quadData;
	std::vector<uint16_t> voxelType;


	void addQuad(int x, int y, int z, int w, int h, int dir, uint16_t voxelData) {
		std::cout << "Face: " << dir << " pos: (" << x << "," << y << "," << z << ") "
			<< "size: (" << w << "x" << h << ")\n";
		uint32_t quadVal = 0;
		quadVal |= (x & 0x1F) << 0;
		quadVal |= (y & 0x1F) << 5;
		quadVal |= (z & 0x1F) << 10;
		quadVal |= (w & 0x1F) << 15;
		quadVal |= (h & 0x1F) << 20;
		quadVal |= (dir & 0x07) << 25;
		
		quadData.push_back(quadVal);
		voxelType.push_back(voxelData);
  	}
};


#endif

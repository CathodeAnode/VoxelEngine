#ifndef VOXEL_WORLD_EDITOR_H
#define VOXEL_WORLD_EDITOR_H

#include <glm/glm.hpp>

#include "types.h"

template <typename ChunkType>
class World;

template <typename ChunkType>
class VoxelWorldEditor
{
public:
	VoxelWorldEditor(World<ChunkType>& context);

	void SetVoxel(const glm::ivec3& coords, RGBAColor color);
	void RemoveVoxel(const glm::ivec3& coords);
	void SetVolume(const Point& A, const Point& B, RGBAColor color);
	void RemoveVolume(const Point& A, const Point& B);
	void SetSphere(glm::ivec3 coords, uint16_t radius, RGBAColor color);
	void RemoveSphere(glm::ivec3 coords, uint16_t radius);

	void SwitchContext(World<ChunkType>& context);

private:
	World<ChunkType>& world;
};

#endif


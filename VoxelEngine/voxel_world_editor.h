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

	void SetBoxVolume(const Point& A, const Point& B, RGBAColor color);
	void RemoveBoxVolume(const Point& A, const Point& B);

	void SetSphere(glm::ivec3 coords, uint16_t radius, RGBAColor color);
	void RemoveSphere(glm::ivec3 coords, uint16_t radius);

	void SwitchContext(World<ChunkType>& context);

private:
	World<ChunkType>& m_World;

	inline static glm::ivec3 _VoxelToChunkPos(const glm::ivec3& voxelWorldPos);
	inline static glm::ivec3 _WorldToLocalPos(const glm::ivec3& voxelWorldPos);
};

#include "voxel_world_editor.tpp"

#endif



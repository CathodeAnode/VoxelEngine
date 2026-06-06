#ifndef __VE_VOXEL_EDIT_H
#define __VE_VOXEL_EDIT_H

#include <algorithm>

#include <glm/glm.hpp>

#include "view_ring_buffer.h"

#include "types.h"
#include "chunk_provider_concept.h"
#include "voxel_math.h"


template<typename ChunkType>
class VoxelRenderer;

template <typename ChunkType, ChunkProvider<ChunkType> ChunkContainer>
class VoxelEdit
{
public:
	explicit VoxelEdit(ChunkContainer& chunkContainer, unsigned int cap);

	ViewRingBuffer<glm::ivec3>::Range GetDirtyChunks(unsigned int budget);

	void SetVoxel(const glm::ivec3& coords, RGBAColor color);
	void RemoveVoxel(const glm::ivec3& coords);

	void SetBoxVolume(const glm::ivec3& A, const glm::ivec3& B, RGBAColor color);
	void RemoveBoxVolume(const glm::ivec3& A, const glm::ivec3& B);

	void SetSphere(glm::ivec3 coords, uint16_t radius, RGBAColor color);
	void RemoveSphere(glm::ivec3 coords, uint16_t radius);
	
	// TODO: implement methods for a unified edit
	//	- methods to being the unified edit, such as BeginEdit and EndEdit
	//	- methods to manipulate chunk data without appending chunks to dirty chunk container

	// High-level concept:
	// - user begins edit with BeginEdit, state is reset and new edit is begun
	// - user manipulates chunk data with methods, dirty chunks are added to an unordered set
	// - user ends edit with EndEdit, all the chunks marked as dirty in unordered set
	//   during unified edit is added to m_DirtyChunks container


private:
	ViewRingBuffer<glm::ivec3> m_DirtyChunks;
	ChunkContainer& m_ChunkContainer;

private:
	void _MarkDirtyChunksForVoxel(const glm::ivec3& chunkCoords, const glm::ivec3& localVoxelCoords);
};

#include "voxel_edit.tpp"

#endif



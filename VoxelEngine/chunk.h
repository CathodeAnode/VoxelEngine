#ifndef CHUNK_H
#define CHUNK_H

#include <cstring>
#include <iostream>
#include <unordered_map>
#include <limits>

#include "types.h"
#include "uid_manager.h"
#include "chunk_provider_concept.h"

template<typename T, unsigned int ChunkSize>
class Chunk {
public:
	using ValueType = T;
	static constexpr unsigned int Size = ChunkSize;

	Chunk();
	~Chunk();

	Chunk(const Chunk& other);
	Chunk& operator=(const Chunk& other);
	Chunk(Chunk&& other) noexcept;
	Chunk& operator=(Chunk&& other) noexcept;

	bool IsSolid(int x, int y, int z) const;
	virtual bool IsEmpty() const;
	void ToggleBit(int x, int y, int z);
	void SetVoxel(int x, int y, int z, RGBAColor color);

	virtual inline VoxelObjectID GetUID() const { return k_Uid; };
	inline RGBAColor GetVoxelData(int x, int y, int z) const { return m_ColorData[_GetColorDataIndex(x, y, z)]; }
	inline RGBAColor GetVoxelData(glm::ivec3 coords) const { return GetVoxelData(coords.x, coords.y, coords.z); }
	virtual inline T GetColumnRow(int x, int z) const { return m_OpaqueData[_GetOpaqueDataIndex(x, z)]; }

	char* serialize();
private:
	T* __restrict m_OpaqueData = nullptr; // 1 for block, 0 for air (z-major order)
	RGBAColor* __restrict m_ColorData = nullptr;
	const VoxelObjectID k_Uid;

private:
	static inline size_t _GetOpaqueDataIndex(int x, int z) { return x + z * ChunkSize; }
	static inline size_t _GetColorDataIndex(int x, int y, int z) { return x + z * ChunkSize + y * ChunkSize * ChunkSize; }

private:
	// friend classes to allow for SIMD optimizations
	template <typename ChunkType, ChunkProvider<ChunkType> ChunkContainer>
	friend class VoxelEdit;

	template<typename ChunkType>
	friend class ChunkGeneratorStrategy;

	template<typename ChunkType>
	friend class EmptyChunkGeneration;

	template<typename ChunkType>
	friend class FlatChunkGeneration;
};

typedef Chunk<uint8_t, 8> Chunk8;
typedef Chunk<uint16_t, 16> Chunk16;
typedef Chunk<uint32_t, 32> Chunk32;


// NullChunk class that behaves as an empty chunk
// this is temporary, figure out a way to create a null chunk / empty chunk without virtual funcs and overrides
template<typename T, unsigned int ChunkSize>
class NullChunk : public Chunk<T, ChunkSize> {
public:
	// Override constructor if needed
	NullChunk()  {}

	T GetColumnRow(int x, int z) const override { return T(0); }
	bool IsEmpty() const override { return true; }
	VoxelObjectID GetUID() const override  { return 0; }

};

#include "chunk.tpp"


#endif // !CHUNK_H

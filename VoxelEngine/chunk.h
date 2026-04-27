#ifndef CHUNK_H
#define CHUNK_H

#include <cstring>
#include <iostream>
#include <unordered_map>
#include <limits>
#include <span>
#include <glm/fwd.hpp>

#include "types.h"
#include "uid_manager.h"
#include "chunk_provider_concept.h"

// TODO algin data for SIMD instructions
template<typename T, unsigned int ChunkSize>
class Chunk {
public:
	using ValueType = T;
	static constexpr size_t Size = ChunkSize;

	Chunk(glm::ivec3 chunkCoords);
	~Chunk();

	Chunk(const Chunk& other);
	Chunk& operator=(const Chunk& other);
	Chunk(Chunk&& other) noexcept;
	Chunk& operator=(Chunk&& other) noexcept;

	bool IsSolid(int x, int y, int z) const;
	bool IsEmpty() const;
	void ToggleBit(int x, int y, int z);
	void SetVoxel(int x, int y, int z, RGBAColor color);

	inline RGBAColor GetVoxelColorAt(int x, int y, int z) const { return m_ColorData[ColorDataIndexAt(x, y, z)]; }
	inline RGBAColor GetVoxelColorAt(glm::ivec3 coords) const { return GetVoxelColorAt(coords.x, coords.y, coords.z); }
	inline T GetColumnRow(int x, int z) const { return m_OpaqueData[OpaqueDataIndexAt(x, z)]; }

	inline VoxelObjectID GetUID() const { return k_Uid; };

	inline T* GetOpaqueData() noexcept { return m_OpaqueData; }
	inline RGBAColor* GetColorData() noexcept { return m_ColorData; }
	inline std::span<T> GetOpaqueSpan() noexcept { return std::span<T>(m_OpaqueData, Size * Size); }
	inline std::span<RGBAColor> GetColorSpan() noexcept { return std::span<RGBAColor>(m_ColorData, Size * Size * Size); }

	inline const T* GetOpaqueData() const noexcept { return m_OpaqueData; }
	inline const RGBAColor* GetColorData() const noexcept { return m_ColorData; }
	inline std::span<const T> GetOpaqueSpan() const noexcept { return std::span<const T>(m_OpaqueData, Size * Size); }
	inline std::span<const RGBAColor> GetColorSpan() const noexcept { return std::span<const RGBAColor>(m_ColorData, Size * Size * Size); }

	static inline size_t OpaqueDataIndexAt(int x, int z) { return x + z * ChunkSize; }
	static inline size_t ColorDataIndexAt(int x, int y, int z) { return x + z * ChunkSize + y * ChunkSize * ChunkSize; }

	char* serialize();
private:
	T* __restrict m_OpaqueData = nullptr; // 1 for block, 0 for air (z-major order)
	RGBAColor* __restrict m_ColorData = nullptr;
	const VoxelObjectID k_Uid;
};

typedef Chunk<uint8_t, 8> Chunk8;
typedef Chunk<uint16_t, 16> Chunk16;
typedef Chunk<uint32_t, 32> Chunk32;

#include "chunk.tpp"


#endif // !CHUNK_H

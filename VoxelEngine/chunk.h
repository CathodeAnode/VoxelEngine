#ifndef CHUNK_H
#define CHUNK_H

#include <cstring>
#include <iostream>
#include <unordered_map>
#include <limits>

#include "types.h"
#include "uid_manager.h"

#define DEFAULT_VOXEL_COLOR 0x808080FF

template<typename T, unsigned int ChunkSize>
class Chunk {
public:
	using ValueType = T;
	static constexpr unsigned int Size = ChunkSize;

	// TODO : remove filled bool, always create empty chunk
	// & Create another consturtor that takes input to generate voxel data in chunk
	Chunk(bool filled=false) 
		: k_Uid(UIDManager::Generate())
	{
		m_OpaqueData = new T[ChunkSize * ChunkSize];
		m_ColorData = new RGBAColor[ChunkSize * ChunkSize * ChunkSize];
		if (filled) {
			std::fill(m_OpaqueData, m_OpaqueData + ChunkSize * ChunkSize, ~T(0));  // set all bits to 1
			std::fill(m_ColorData, m_ColorData + ChunkSize * ChunkSize * ChunkSize, DEFAULT_VOXEL_COLOR);
		}
		//else
		//{
		//	std::fill(m_OpaqueData, m_OpaqueData + ChunkSize * ChunkSize, T(0));
		//	std::fill(m_ColorData, m_ColorData + ChunkSize * ChunkSize * ChunkSize, RGBAColor(0));
		//}
		
	};

	// Copy constructor
	Chunk(const Chunk& other)
		: k_Uid(UIDManager::Generate())
	{
		m_OpaqueData = new T[ChunkSize * ChunkSize];
		m_ColorData = new RGBAColor[ChunkSize * ChunkSize * ChunkSize];

		std::memcpy(m_OpaqueData, other.m_OpaqueData, ChunkSize * ChunkSize * sizeof(T));
		std::memcpy(m_ColorData, other.m_ColorData, ChunkSize * ChunkSize * ChunkSize * sizeof(RGBAColor));
	}

	// copy assignment
	Chunk& operator=(const Chunk& other) 
	{
		if (this != &other) 
		{
			delete[] m_OpaqueData;
			delete[] m_ColorData;

			m_OpaqueData = new T[ChunkSize * ChunkSize];
			m_ColorData = new RGBAColor[ChunkSize * ChunkSize * ChunkSize];

			std::memcpy(m_OpaqueData, other.m_OpaqueData, ChunkSize * ChunkSize * sizeof(T));
			std::memcpy(m_ColorData, other.m_ColorData, ChunkSize * ChunkSize * ChunkSize * sizeof(RGBAColor));
		}
		return *this;
	}

	// Move constructor
	Chunk(Chunk&& other) noexcept
		: k_Uid(other.k_Uid)
	{
		delete[] m_OpaqueData;
		delete[] m_ColorData;

		m_OpaqueData = std::move(other.m_OpaqueData);
		m_ColorData = std::move(other.m_ColorData);

		other.m_OpaqueData = nullptr;
		other.m_ColorData = nullptr;
	}

	// Move assignment
	Chunk& operator=(Chunk&& other) noexcept 
	{
		if (this != &other) 
		{
			delete[] m_OpaqueData;
			delete[] m_ColorData;

			m_OpaqueData = std::move(other.m_OpaqueData);
			m_ColorData = std::move(other.m_ColorData);

			other.m_OpaqueData = nullptr;
			other.m_ColorData = nullptr;
		}
		return *this;
	}

	~Chunk() 
	{
		if (m_OpaqueData) 
		{
			delete[] m_OpaqueData;
			delete[] m_ColorData;
		}
	};
	
	bool IsSolid(int x, int y, int z) const 
	{
		int rowIndex = x + y * ChunkSize;
		return m_OpaqueData[rowIndex] << z;
	}
	
	virtual bool IsEmpty() const 
	{
		for (int i = 0; i < ChunkSize * ChunkSize; i++) 
		{
			if (m_OpaqueData[i] != 0) {
				return false;
			}
		}
		return true;
	}

	inline RGBAColor GetVoxelData(int x, int y, int z) const
	{
		return m_ColorData[x + z * ChunkSize + y * ChunkSize * ChunkSize];
	}

	inline RGBAColor GetVoxelData(glm::ivec3 coords) const
	{
		return GetVoxelData(coords.x, coords.y, coords.z);
	}

	virtual inline T GetColumnRow(int x, int z) const
	{
		return m_OpaqueData[x + (z * ChunkSize)];
	}

	void ToggleBit(int x, int y, int z) 
	{
		int index = x + z * ChunkSize;
		m_OpaqueData[index] ^= (T(1) << y);
	}

	void SetVoxel(int x, int y, int z, RGBAColor color)
	{
		m_OpaqueData[x + z * ChunkSize] |= (T(1) << y);
		m_ColorData[x + z * ChunkSize + y * ChunkSize * ChunkSize] = color;
	}

	void printData() 
	{

		unsigned int bits_per_element = sizeof(T) * 8;

		for (unsigned int y = 0; y < ChunkSize; ++y) 
		{
			for (unsigned int x = 0; x < ChunkSize; ++x) 
			{
				T val = m_OpaqueData[x + y * ChunkSize];
				// print bits in val from most significant to least significant
				for (int bit = bits_per_element - 1; bit >= 0; --bit) 
				{
					bool bit_set = (val & (T(1) << bit)) != 0;
					std::cout << (bit_set ? '1' : '0');
				}
				std::cout << ' ';  // space between elements for clarity
			}
			std::cout << std::endl;
		}
	}

	virtual inline VoxelObjectID GetUID() const { return k_Uid; };

	char* serialize();
private:
	T* m_OpaqueData = nullptr; // 1 for block, 0 for air (z-major order)
	RGBAColor* m_ColorData = nullptr;
	const VoxelObjectID k_Uid;

	template<typename ChunkType>
	friend class VoxelWorldEditor;
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

	T GetColumnRow(int x, int z) const override
	{
		return T(0);
	}

	bool IsEmpty() const override
	{
		return true;
	}

	VoxelObjectID GetUID() const override 
	{ 
		return 0; 
	}

	// TODO override other methods if needed
};


#endif // !CHUNK_H

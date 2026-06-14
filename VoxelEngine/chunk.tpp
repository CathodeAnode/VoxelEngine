#ifndef CHUNK_TPP
#define CHUNK_TPP

#include "chunk.h"

template<typename T, unsigned int ChunkSize>
Chunk<T, ChunkSize>::Chunk(glm::ivec3 chunkCoords)
	: k_Uid(UIDManager::Generate(chunkCoords))
{
	m_OpaqueData = new T[ChunkSize * ChunkSize];
	m_ColorData = new RGBAColor[ChunkSize * ChunkSize * ChunkSize];

	std::fill(m_OpaqueData, m_OpaqueData + ChunkSize * ChunkSize, T(0));
	std::fill(m_ColorData, m_ColorData + ChunkSize * ChunkSize * ChunkSize, RGBAColor(0));
};

template<typename T, unsigned int ChunkSize>
Chunk<T, ChunkSize>::~Chunk()
{
	if (m_OpaqueData)
	{
		delete[] m_OpaqueData;
		delete[] m_ColorData;
	}
};


// Copy constructor
template<typename T, unsigned int ChunkSize>
Chunk<T, ChunkSize>::Chunk(const Chunk& other)
	: k_Uid(UIDManager::Generate())
{
	m_OpaqueData = new T[ChunkSize * ChunkSize];
	m_ColorData = new RGBAColor[ChunkSize * ChunkSize * ChunkSize];

	std::memcpy(m_OpaqueData, other.m_OpaqueData, ChunkSize * ChunkSize * sizeof(T));
	std::memcpy(m_ColorData, other.m_ColorData, ChunkSize * ChunkSize * ChunkSize * sizeof(RGBAColor));
}

// copy assignment
template<typename T, unsigned int ChunkSize>
Chunk<T, ChunkSize>& Chunk<T, ChunkSize>::operator=(const Chunk<T, ChunkSize>& other)
{
	if (this != &other)
	{
		delete[] m_OpaqueData;
		delete[] m_ColorData;

		m_OpaqueData = new T[ChunkSize * ChunkSize];
		m_ColorData = new RGBAColor[ChunkSize * ChunkSize * ChunkSize];
		k_Uid = UIDManager::Generate();

		std::memcpy(m_OpaqueData, other.m_OpaqueData, ChunkSize * ChunkSize * sizeof(T));
		std::memcpy(m_ColorData, other.m_ColorData, ChunkSize * ChunkSize * ChunkSize * sizeof(RGBAColor));
	}
	return *this;
}

// Move constructor
template<typename T, unsigned int ChunkSize>
Chunk<T, ChunkSize>::Chunk(Chunk&& other) noexcept
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
template<typename T, unsigned int ChunkSize>
Chunk<T, ChunkSize>& Chunk<T, ChunkSize>::operator=(Chunk<T, ChunkSize>&& other) noexcept
{
	delete[] m_OpaqueData;
	delete[] m_ColorData;

	m_OpaqueData = std::move(other.m_OpaqueData);
	m_ColorData = std::move(other.m_ColorData);
	k_Uid = other.k_Uid;

	other.m_OpaqueData = nullptr;
	other.m_ColorData = nullptr;
}


template<typename T, unsigned int ChunkSize>
bool Chunk<T, ChunkSize>::IsSolid(int x, int y, int z) const
{
	return (m_OpaqueData[OpaqueDataIndexAt(x, z)] & (T(1) << y)) != 0;
}

template<typename T, unsigned int ChunkSize>
bool Chunk<T, ChunkSize>::IsEmpty() const
{
	for (int i = 0; i < ChunkSize * ChunkSize; i++)
	{
		if (m_OpaqueData[i] != 0) {
			return false;
		}
	}
	return true;
}

template<typename T, unsigned int ChunkSize>
void Chunk<T, ChunkSize>::ToggleBit(int x, int y, int z)
{
	m_OpaqueData[OpaqueDataIndexAt(x, z)] ^= (T(1) << y);
}

template<typename T, unsigned int ChunkSize>
void Chunk<T, ChunkSize>::SetVoxel(int x, int y, int z, RGBAColor color)
{
	m_OpaqueData[OpaqueDataIndexAt(x, z)] |= (T(1) << y);
	m_ColorData[ColorDataIndexAt(x, y, z)] = color;
}

template<typename T, unsigned int ChunkSize>
void Chunk<T, ChunkSize>::ClearVoxel(int x, int y, int z)
{
	m_OpaqueData[OpaqueDataIndexAt(x, z)] |= (T(0) << y);
}


#endif
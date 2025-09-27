#ifndef CHUNK_GENERATOR_STRATEGY_H
#define CHUNK_GENERATOR_STRATEGY_H

#include <glm/glm.hpp>

template<typename ChunkType>
class ChunkGeneratorStrategy
{
public:
	virtual ~ChunkGeneratorStrategy() = default;
	virtual void Generate(const glm::ivec3& chunkCoords, ChunkType& outChunk) = 0;
};

template<typename ChunkType>
class EmptyChunkGeneration : public ChunkGeneratorStrategy<ChunkType>
{
public:
	void Generate(const glm::ivec3 chunkCoords, ChunkType& outChunk) override
	{
		// TODO: SIMD me
		for (int i = 0; i < ChunkType::Size * ChunkType::Size; i++)
		{
			outChunk.m_OpaqueData[i] = 0;
		}
	}
};

template<typename ChunkType>
class FlatChunkGeneration : public ChunkGeneratorStrategy<ChunkType>
{
public:
	// TODO: pass some sort of struct/class to specifiy the color generation of the voxels
	FlatChunkGeneration(int heightLevel)
		: k_HeightLevel(heightLevel)
	{}

	void Generat(const glm::ivec3& chunkCoords, ChunkType& outChunk)
	{
		ChunkType::ValueType voxelFill;
		if (chunkCoords.y > k_HeightLevel)
		{
			voxelFill = 0;
		}
		else
		{
			voxelFill = ~ChunkType::ValueType(0);
			// TODO: set voxel color data using passed object in constructor
			for (int i = 0; i < ChunkType::Size * ChunkType::Size * ChunkType::Size; i++)
			{
				outChunk.m_ColorData[i] = 0xefefefff; // white color
			}
		}

		// TODO: SIMD me
		for (int i = 0; i < ChunkType::Size * ChunkType::Size; i++)
		{
			outChunk.m_OpaqueData[i] = voxelFill;
		}
	}

private:
	const int k_HeightLevel;
};


#endif
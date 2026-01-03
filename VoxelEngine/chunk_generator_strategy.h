#ifndef CHUNK_GENERATOR_STRATEGY_H
#define CHUNK_GENERATOR_STRATEGY_H

#include <memory>
#include <string.h>

// Forward declare glm::ivec3
#include <glm/fwd.hpp> 

#include "perlin_noise.h"

//TEMP
#include <random>
#include <chrono>

template<typename ChunkType>
class ChunkGeneratorStrategy
{
public:
	//TEMP
	ChunkGeneratorStrategy()
		: p_Seed(std::chrono::system_clock::now().time_since_epoch().count())
		, p_Engine(p_Seed)
		, p_Dist(0, 0xffffffff)
	{}

	virtual ~ChunkGeneratorStrategy() = default;
	virtual void Generate(const glm::ivec3& chunkCoords, const std::shared_ptr<ChunkType>& outChunk) = 0;

	virtual std::string ToString() = 0;

protected:
	//TEMP
	unsigned p_Seed;
	std::mt19937 p_Engine;
	std::uniform_int_distribution<unsigned long int> p_Dist;
};

template<typename ChunkType>
class EmptyChunkGeneration : public ChunkGeneratorStrategy<ChunkType>
{
public:
	void Generate(const glm::ivec3& chunkCoords, const std::shared_ptr<ChunkType>& outChunk) override
	{
		constexpr int totalSize = ChunkType::Size * ChunkType::Size;

		// TODO: SIMD me
		for (int i = 0; i < totalSize; ++i)
		{
			outChunk->m_OpaqueData[i] = 0;
		}
	}

	std::string ToString() override { return "EmptyChunkGenertation"; }
};

template<typename ChunkType>
class FlatChunkGeneration : public ChunkGeneratorStrategy<ChunkType>
{
public:
	// TODO: pass some sort of struct/class to specifiy the color generation of the voxels
	FlatChunkGeneration(int heightLevel)
		: k_HeightLevel(heightLevel)
	{}

	void Generate(const glm::ivec3& chunkCoords, const std::shared_ptr<ChunkType>& outChunk)
	{
		typename ChunkType::ValueType voxelFill;
		if (chunkCoords.y >= k_HeightLevel)
		{
			voxelFill = 0;
		}
		else
		{
			voxelFill = ~ChunkType::ValueType(0);
			// TODO: set voxel color data using passed object in constructor

			//TEMP
			const int randColor = this->p_Dist(this->p_Engine);

			auto chunkColorData = outChunk->GetColorSpan();
			for (int i = 0; i < chunkColorData.size(); i++)
			{
				chunkColorData[i] = randColor;
			}
		}

		// TODO: SIMD me
		auto chunkOpaqueData = outChunk->GetOpaqueSpan();
		for (int i = 0; i < chunkOpaqueData.size(); i++)
		{
			chunkOpaqueData[i] = voxelFill;
		}
	}

	std::string ToString() override { return "FlatChunkGenertation"; }

private:
	const int k_HeightLevel;
};

template<typename ChunkType>
class SinusoidalChunkGeneration : public ChunkGeneratorStrategy<ChunkType>
{
public:
	SinusoidalChunkGeneration(int height)
		: m_Amplitude(height)
	{}


	void Generate(const glm::ivec3& chunkCoords, const std::shared_ptr<ChunkType>& outChunk)
	{
		const float freq = 0.10f;

		//TEMP
		const int randColor = this->p_Dist(this->p_Engine);

		for (int x = 0; x < ChunkType::Size; x++)
		{
			for (int y = 0; y < ChunkType::Size; y++)
			{
				for (int z = 0; z < ChunkType::Size; z++)
				{
					int worldX = chunkCoords.x * ChunkType::Size + x;
					int worldY = chunkCoords.y * ChunkType::Size + y;
					int worldZ = chunkCoords.z * ChunkType::Size + z;

					float height = m_Amplitude * sin(worldX * freq) * sin(worldZ * freq);

					if (worldY < height)
					{
						outChunk->SetVoxel(x, y, z, randColor);
					}
				}
			}
		}
	}

	std::string ToString() override { return "SinusoidalChunkGeneration"; }

private:
	const int m_Amplitude;
};

template<typename ChunkType>
class Simple3DPerlinNoiseGeneration : public ChunkGeneratorStrategy<ChunkType>
{
public:
	Simple3DPerlinNoiseGeneration()
		: m_PerlinNoise(this->p_Seed)
	{}

	void Generate(const glm::ivec3& chunkCoords, const std::shared_ptr<ChunkType>& outChunk) override
	{
		constexpr int totalSize = ChunkType::Size * ChunkType::Size;

		//TEMP
		const int randColor = this->p_Dist(this->p_Engine);

		for (int x = 0; x < ChunkType::Size; x++)
		{
			for (int y = 0; y < ChunkType::Size; y++)
			{
				for (int z = 0; z < ChunkType::Size; z++)
				{
					int worldX = chunkCoords.x * ChunkType::Size + x;
					int worldY = chunkCoords.y * ChunkType::Size + y;
					int worldZ = chunkCoords.z * ChunkType::Size + z;

					float noiseVal = m_PerlinNoise.Noise3D(worldX * 0.1, worldY * 0.1, worldZ * 0.1);

					if (noiseVal > 0.2f)
					{
						outChunk->SetVoxel(x, y, z, randColor);
					}
				}
			}
		}
	}

	std::string ToString() override { return "Simple3DPerlinNoiseGeneration"; }

private:
	PerlinNoise m_PerlinNoise;
};


#endif
#ifndef CHUNK_GENERATOR_STRATEGY_H
#define CHUNK_GENERATOR_STRATEGY_H

#include <memory>

// Forward declare glm::ivec3
#include <glm/fwd.hpp> 


//TEMP
#include <random>
#include <chrono>

template<typename ChunkType>
class ChunkGeneratorStrategy
{
public:
	virtual ~ChunkGeneratorStrategy() = default;
	virtual void Generate(const glm::ivec3& chunkCoords, const std::shared_ptr<ChunkType>& outChunk) = 0;
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
};

template<typename ChunkType>
class FlatChunkGeneration : public ChunkGeneratorStrategy<ChunkType>
{
public:
	// TODO: pass some sort of struct/class to specifiy the color generation of the voxels
	FlatChunkGeneration(int heightLevel)
		: k_HeightLevel(heightLevel)
		, seed(std::chrono::system_clock::now().time_since_epoch().count())
		, engine(seed)
		, dist(0, 0xffffffff)
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
			const int randColor = dist(engine);

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

private:
	const int k_HeightLevel;

	//TEMP
	unsigned seed;
	std::mt19937 engine;
	std::uniform_int_distribution<unsigned long int> dist;
};

template<typename ChunkType>
class SinusoidalChunkGeneration : public ChunkGeneratorStrategy<ChunkType>
{
public:
	SinusoidalChunkGeneration(int height)
		: m_Amplitude(height)
		, seed(std::chrono::system_clock::now().time_since_epoch().count())
		, engine(seed)
		, dist(0, 0xffffffff)
	{}


	void Generate(const glm::ivec3& chunkCoords, const std::shared_ptr<ChunkType>& outChunk)
	{
		const float freq = 0.10f;

		//TEMP
		const int randColor = dist(engine);

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

private:
	const int m_Amplitude;

	//TEMP
	unsigned seed;
	std::mt19937 engine;
	std::uniform_int_distribution<unsigned long int> dist;
};


#endif
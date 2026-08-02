#ifndef CHUNK_GENERATOR_STRATEGY_H
#define CHUNK_GENERATOR_STRATEGY_H

#include <memory>
#include <string.h>

// Forward declare glm::ivec3
#include <glm/fwd.hpp> 

#include "stb_image.h"

#include "perlin_noise.h"
#include "voxel_math.h"
#include "logger.h"
#include "profiler.h"

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

		std::memset(outChunk->m_OpaqueData, 0, totalSize);
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

		auto chunkOpaqueData = outChunk->GetOpaqueSpan();
		std::fill(chunkOpaqueData.begin(), chunkOpaqueData.end(), voxelFill);
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

					float noiseVal = m_PerlinNoise.Noise3D(worldX * 0.1f, worldY * 0.1f, worldZ * 0.1f);

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

template<typename ChunkType>
class HeightmapChunkGeneration : public ChunkGeneratorStrategy<ChunkType>
{
public:
	using ValueType = typename ChunkType::ValueType;

	HeightmapChunkGeneration(const std::string& heightmapPath, float worldHeightScale)
		: m_WorldHeightScale(worldHeightScale)
	{
		PROFILE_FUNCTION();

		int width, height, channels;

		// Force grayscale load (1 channel)
		unsigned char* data = stbi_load(
			heightmapPath.c_str(),
			&width,
			&height,
			&channels,
			1
		);

		if (!data)
		{

			LOG_ERROR(EngineSystem::VOXEL_ENGINE, "Failed to load heightmap: {}", heightmapPath);
			throw std::runtime_error("Failed to load heightmap: " + heightmapPath);
		}

		m_Width = width;
		m_Height = height;

		m_Data.resize(width * height);

		// Convert to float [0,1]
		for (int i = 0; i < width * height; i++)
		{
			m_Data[i] = data[i] / 255.0f;
		}

		stbi_image_free(data);

		// Set world bounds assuming 1 pixel = 1 voxel * voxel size
		m_WorldBoundsMin = glm::ivec3(
			-width / 2,
			0.0f,
			-height / 2
		);
		m_WorldBoundsMax = glm::vec3(
			width / 2,
			0.0f,
			height / 2
		);
	}


	void Generate(const glm::ivec3& chunkCoords, const std::shared_ptr<ChunkType>& outChunk) override
	{
		PROFILE_FUNCTION();

		auto opaqueSpan = outChunk->GetOpaqueSpan();
		auto colorSpan = outChunk->GetColorSpan();

		const int chunkSize = ChunkType::Size;

		//TEMP
		const int randColor = this->p_Dist(this->p_Engine);

		// World origin of this chunk (in meters)
		glm::vec3 chunkWorldOrigin = ChunkToWorldOrigin(chunkCoords, chunkSize);

		for (int z = 0; z < chunkSize; z++)
			for (int x = 0; x < chunkSize; x++)
			{
				// World position (XZ plane)
				float worldX = chunkWorldOrigin.x + x;
				float worldZ = chunkWorldOrigin.z + z;

				// Convert to normalized heightmap UV (0..1)
				float u = (worldX - m_WorldBoundsMin.x) / (m_WorldBoundsMax.x - m_WorldBoundsMin.x);
				float v = (worldZ - m_WorldBoundsMin.z) / (m_WorldBoundsMax.z - m_WorldBoundsMin.z);

				float height = _SampleHeightmap(u, v) * m_WorldHeightScale;

				// Fill vertical column in chunk
				for (int y = 0; y < chunkSize; y++)
				{
					float worldY = chunkWorldOrigin.y + y;


					if (worldY <= height)
					{
						outChunk->SetVoxel(x, y, z, randColor);
					}
					else
					{
						outChunk->ClearVoxel(x, y, z);
					}
				}
			}
	}

	std::string ToString() override { return "HeightmapChunkGeneration";}

private:
	float _SampleHeightmap(float u, float v) const
	{
		int x = (int)(u * (m_Width - 1));
		int y = (int)(v * (m_Height - 1));

		int x2 = std::min(x + 1, m_Width - 1);
		int y2 = std::min(y + 1, m_Height - 1);

		float tx = (u * (m_Width - 1)) - x;
		float ty = (v * (m_Height - 1)) - y;

		float h00 = m_Data[y * m_Width + x];
		float h10 = m_Data[y * m_Width + x2];
		float h01 = m_Data[y2 * m_Width + x];
		float h11 = m_Data[y2 * m_Width + x2];

		float hx0 = glm::mix(h00, h10, tx);
		float hx1 = glm::mix(h01, h11, tx);

		return glm::mix(hx0, hx1, ty);
	}

private:
	std::vector<float> m_Data;
	int m_Width = 0;
	int m_Height = 0;

	float m_WorldHeightScale; // max terrain height in meters

	glm::ivec3 m_WorldBoundsMin;
	glm::ivec3 m_WorldBoundsMax;
};


#endif
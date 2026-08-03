#ifndef PERLIN_NOISE_H
#define PERLIN_NOISE_H

#include <iterator>
#include <algorithm>
#include <random>
#include <concepts>
#include <functional>

struct PerlinNoiseConfig
{
	int octves = 8;
	float lacunarity = 2.0f;
	float gain = 0.5f;
	float amplitude = 1.0f;
	float freq = 1.0f;
};

class PerlinNoise
{
public:
	PerlinNoise(unsigned int seed);

	void Reseed(unsigned int seed);

	float Noise1D(float x);
	float Noise2D(float x, float y);
	float Noise3D(float x, float y, float z);

	// example usage: obj.AccumulatedNoise(PerlinNoiseConfig{}, &PerlinNoise::Noise3D, 0, 1, 2);
	template <typename NoiseFn, typename... Args>
	requires std::invocable<NoiseFn, PerlinNoise&, Args...> && std::convertible_to<std::invoke_result_t<NoiseFn, PerlinNoise&, Args...>, float>
	float AccumulatedNoise(PerlinNoiseConfig cfg, NoiseFn noiseFn, Args... args)
	{
		float result = 0.0f;
		float maxVal = 0.0f;

		for (; cfg.octves > 0; --cfg.octves)
		{
			result += std::invoke(noiseFn, *this, (args * cfg.freq)...) * cfg.amplitude;
			maxVal += cfg.amplitude;

			cfg.amplitude *= cfg.gain;
			cfg.freq *= cfg.lacunarity;
		}

		return result / maxVal;
	}

	// SIMD functions
	void SIMD_noise2D(float x, float y, float* out);
	void SIMD_noise3D(float x, float y, float z, float* out);

private:
	//psudo-random permutation table
	unsigned char m_PTable[512];

	unsigned int m_Seed;

private:
	// fade function f(t) = 6*t^5 - 15*t^4 + 10*t^3 (optimize for fewer multiplications)
	inline static float fade(float t)
	{
		return t * t * t * (t * (t * 6 - 15) + 10);
	}

	//linear interpolation
	inline static float lerp(float t, float a, float b)
	{
		return a + t * (b - a);
	}

	inline static float map(float val, float currMin, float currMax, float newMin, float newMax)
	{
		float prop = (val - currMin) / (currMin - currMax);

		return lerp(prop, newMin, newMax);
	}

	// Calculate the dot product between the gradient vector and distance vector
	inline static float gradient(unsigned int hash, float x, float y, float z)
	{
	    // Use 4 bits to select one of 12 gradient directions
		int h = hash & 0xF;

		float u = (h < 8) ? x : y;
		float v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
		
		// Apply sign bits
		if (h & 1) u = -u;
		if (h & 2) v = -v;
		
		return u + v;
	}

	// 1D hash function
	inline static unsigned int Hash1D(int x, unsigned int seed)
	{
		unsigned int h = static_cast<unsigned int>(x) + seed;
		h = (h ^ 61) ^ (h >> 16);
		h = h + (h << 3);
		h = h ^ (h >> 4);
		h = h * 0x27d4eb2dU;
		h = h ^ (h >> 15);
		return h;
	}

	// 2D hash function
	inline static unsigned int Hash2D(int x, int y, unsigned int seed)
	{
		unsigned int h = static_cast<unsigned int>(x * 1664525U + y * 1013904223U + seed * 0x9e3779b9U);
		h ^= h >> 16;
		h *= 0x85ebca6bU;
		h ^= h >> 13;
		h *= 0xc2b2ae35U;
		h ^= h >> 16;
		return h;
	}

	// 3D hash function
	inline static unsigned int Hash3D(int x, int y, int z, unsigned int seed)
	{
		unsigned int h = static_cast<unsigned int>(x * 374761393U + y * 668265263U + z * 1274126177U + seed * 0x9e3779b9U);
		h ^= h >> 13;
		h *= 0xbf58476dU;
		h ^= h >> 15;
		h *= 0x94d049bbU;
		h ^= h >> 16;
		return h;
	}
};

#endif


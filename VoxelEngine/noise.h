#ifndef NOISE_H
#define NOISE_H

#include <iterator>
#include <algorithm>
#include <random>
#include <concepts>
#include <functional>
#include <cstdint>

struct NoiseConfig
{
	int octves = 8;
	float lacunarity = 2.0f;
	float gain = 0.5f;
	float amplitude = 1.0f;
	float freq = 1.0f;
};

class Noise
{
public:
	Noise(uint32_t seed);

	void Reseed(uint32_t seed);

	[[nodiscard]] float PerlinNoise1D(float x);
	[[nodiscard]] float PerlinNoise2D(float x, float y);
	[[nodiscard]] float PerlinNoise3D(float x, float y, float z);

	[[nodiscard]] float SimplexNoise2D(float x, float y);
	[[nodiscard]] float SimplexNoise3D(float x, float y, float z);

	// example usage: obj.AccumulatedNoise(NoiseConfig{}, &Noise::SimplexNoise3D, 0, 1, 2);
	template <typename NoiseFn, typename... Args>
	requires std::invocable<NoiseFn, Noise&, Args...> && std::convertible_to<std::invoke_result_t<NoiseFn, Noise&, Args...>, float>
	float AccumulatedNoise(NoiseConfig cfg, NoiseFn noiseFn, Args... args)
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
	void SIMD_PerlinNoise2D(float x, float y, float* out);
	void SIMD_PerlinNoise3D(float x, float y, float z, float* out);

	void SIMD_SimplexNoise2D(float x, float y, float* out);
	void SIMD_SimplexNoise3D(float x, float y, float z, float* out);

private:
	uint32_t m_Seed;

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
	inline static float PerlinGradient(uint32_t hash, float x, float y, float z)
	{
	    // Use 4 bits to select one of 12 gradient directions
		int32_t h = hash & 0xF;

		float u = (h < 8) ? x : y;
		float v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
		
		// Apply sign bits
		if (h & 1) u = -u;
		if (h & 2) v = -v;
		
		return u + v;
	}

	inline static void SimplexGradient2D(uint32_t hash, float& gradientX, float& gradientY)
	{
		const float gradients[8][2] = {
			{ 1.0f,  0.0f},
			{-1.0f,  0.0f},
			{ 0.0f,  1.0f},
			{ 0.0f, -1.0f},
			{ 0.7071067811865475f,  0.7071067811865475f},
			{-0.7071067811865475f,  0.7071067811865475f},
			{ 0.7071067811865475f, -0.7071067811865475f},
			{-0.7071067811865475f, -0.7071067811865475f}
		};

		// Transform hash into index
    	uint32_t index = hash & 7;

		gradientX = gradients[index][0];
		gradientY = gradients[index][1];
	}

	inline static void SimplexGradient3D(uint32_t hash, float& gradientX, float& gradientY, float& gradientZ)
	{
		constexpr float gradients[12][3] = {
			{ 1.0f,  1.0f,  0.0f},
			{-1.0f,  1.0f,  0.0f},
			{ 1.0f, -1.0f,  0.0f},
			{-1.0f, -1.0f,  0.0f},
			{ 1.0f,  0.0f,  1.0f},
			{-1.0f,  0.0f,  1.0f},
			{ 1.0f,  0.0f, -1.0f},
			{-1.0f,  0.0f, -1.0f},
			{ 0.0f,  1.0f,  1.0f},
			{ 0.0f, -1.0f,  1.0f},
			{ 0.0f,  1.0f, -1.0f},
			{ 0.0f, -1.0f, -1.0f}
		};

		uint32_t index = hash % 12;

		gradientX = gradients[index][0];
		gradientY = gradients[index][1];
		gradientZ = gradients[index][2];
	}

	// 1D hash function
	inline static uint32_t Hash1D(int32_t x, uint64_t seed)
	{
		uint64_t h = static_cast<uint64_t>(x) + seed + 0x9e3779b97f4a7c15ULL;
		h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ULL;
		h = (h ^ (h >> 27)) * 0x94d049bb133111ebULL;
		return static_cast<uint32_t>(h ^ (h >> 31));
	}

	// 2D hash function
	inline static uint32_t Hash2D(int32_t x, int32_t y, uint64_t seed)
	{
		uint64_t h = seed + 0x9e3779b97f4a7c15ULL;
		h ^= static_cast<uint64_t>(x) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
		h ^= static_cast<uint64_t>(y) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
		
		h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ULL;
		h = (h ^ (h >> 27)) * 0x94d049bb133111ebULL;
		return static_cast<uint32_t>(h ^ (h >> 31));
	}

	// 3D hash function
	inline static uint32_t Hash3D(int32_t x, int32_t y, int32_t z, uint64_t seed)
	{
		uint64_t h = seed + 0x9e3779b97f4a7c15ULL;
		h ^= static_cast<uint64_t>(x) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
		h ^= static_cast<uint64_t>(y) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
		h ^= static_cast<uint64_t>(z) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
		
		h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ULL;
		h = (h ^ (h >> 27)) * 0x94d049bb133111ebULL;
		return static_cast<uint32_t>(h ^ (h >> 31));
	}

};

#endif


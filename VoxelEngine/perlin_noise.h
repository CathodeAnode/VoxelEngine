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

	//calculate the dot product between the gradient vector and distance vector
	inline static float gradient(unsigned char hash, float x, float y, float z)
	{
		// convert 4 lsb of hash into 1 of 12 possible gradients
		int h = hash & 0b1111;

		// if msb is set u=x else u=y
		float u = h < 01000 ? x : y;

		// if first/second bits 0, set to y
		// if first/second bits 1, set to x
		// else set to z
		float v = h < 0b0100 ? y : h == 0b1100 || h == 0b1110 ? x : z;

		return ((h & 0b0001) == 0 ? u : -u) + ((h & 0b0010) == 0 ? v : -v);
	}
};

#endif


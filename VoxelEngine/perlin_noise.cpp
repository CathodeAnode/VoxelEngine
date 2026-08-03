#include "perlin_noise.h"

PerlinNoise::PerlinNoise(unsigned int seed)
{
    Reseed(seed);
}

void PerlinNoise::Reseed(unsigned int seed)
{
    m_Seed = seed;
}

float PerlinNoise::Noise1D(float x)
{
    int xf = std::floor(x);

    int xi = static_cast<int>(xf);
    
    x -= xf;
    float sx = fade(x);

    unsigned int a = Hash1D(xi, m_Seed);
    unsigned int b = Hash1D(xi + 1, m_Seed);

    float avg = lerp(
        sx,
        gradient(a, x, 0, 0),
        gradient(b, x - 1, 0, 0)
    );

    return avg;
}

float PerlinNoise::Noise2D(float x, float y)
{
    float fx = std::floor(x);
    float fy = std::floor(y);

    int xi = static_cast<int>(fx);
    int yi = static_cast<int>(fy);

    x -= fx;
    y -= fy;

    float sx = fade(x);
    float sy = fade(y);

    unsigned int aa = Hash2D(xi, yi, m_Seed);
    unsigned int ab = Hash2D(xi, yi + 1, m_Seed);
    unsigned int ba = Hash2D(xi + 1, yi, m_Seed);
    unsigned int bb = Hash2D(xi + 1, yi + 1, m_Seed);

    float avg = lerp(
        sy,
        lerp(
            sx,
            gradient(aa, x, y, 0),
            gradient(ba, x - 1, y, 0)
        ),
        lerp(
            sx,
            gradient(ab, x, y - 1, 0),
            gradient(bb, x - 1, y - 1, 0)
        )
    );

    return avg;
}

float PerlinNoise::Noise3D(float x, float y, float z)
{
    float fx = std::floor(x);
    float fy = std::floor(y);
    float fz = std::floor(z);

    int xi = static_cast<int>(fx);
    int yi = static_cast<int>(fy);
    int zi = static_cast<int>(fz);

    x -= fx;
    y -= fy;
    z -= fz;

    float sx = fade(x);
    float sy = fade(y);
    float sz = fade(z);

    unsigned int aaa = Hash3D(xi, yi, zi, m_Seed);
    unsigned int baa = Hash3D(xi + 1, yi, zi, m_Seed);
    unsigned int aba = Hash3D(xi, yi + 1, zi, m_Seed);
    unsigned int bba = Hash3D(xi + 1, yi + 1, zi, m_Seed);
    unsigned int aab = Hash3D(xi, yi, zi + 1, m_Seed);
    unsigned int bab = Hash3D(xi + 1, yi, zi + 1, m_Seed);
    unsigned int abb = Hash3D(xi, yi + 1, zi + 1, m_Seed);
    unsigned int bbb = Hash3D(xi + 1, yi + 1, zi + 1, m_Seed);

    float avg = lerp(
        sz,
        lerp( // front 
            sy,
            lerp( // top
                sx,
                gradient(aaa, x, y, z),
                gradient(baa, x - 1, y, z)
            ),
            lerp( // bottom
                sx,
                gradient(aba, x, y - 1, z),
                gradient(bba, x - 1, y - 1, z)
            )
        ),
        lerp( //rear
            sy,
            lerp( // top
                sx,
                gradient(aab, x, y, z - 1),
                gradient(bab, x - 1, y, z)
            ),
            lerp( // bottom
                sx,
                gradient(abb, x, y - 1, z - 1),
                gradient(bbb, x - 1, y - 1, z - 1)
            )
        )
    );

    return avg;
}

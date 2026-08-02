#include "perlin_noise.h"

PerlinNoise::PerlinNoise(unsigned int seed)
{
    Reseed(seed);
}

void PerlinNoise::Reseed(unsigned int seed)
{
    m_Seed = seed;
    for (unsigned int i = 0; i < 256; i++)
    {
        m_PTable[i] = i;
    }

    std::shuffle(std::begin(m_PTable), std::begin(m_PTable) + 256, std::default_random_engine(m_Seed));

    // duplicate array for overflow
    for (unsigned int i = 0; i < 256; i++)
    {
        m_PTable[i + 256] = m_PTable[i];
    }
}

float PerlinNoise::Noise1D(float x)
{
    int xf = std::floor(x);

    int xi = static_cast<int>(xf) & 255; // = % 256
    
    x -= xf;
    float sx = fade(x);

    const unsigned char* P = m_PTable;

    unsigned char a, b;
    a = P[xi];
    b = P[xi + 1];

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

    int xi = static_cast<int>(fx) & 255;
    int yi = static_cast<int>(fy) & 255;

    x -= fx;
    y -= fy;

    float sx = fade(x);
    float sy = fade(y);

    const unsigned char* P = m_PTable;

    unsigned char aa, ab, ba, bb;
    aa = P[P[xi] + yi];
    ab = P[P[xi] + yi + 1];
    ba = P[P[xi + 1] + yi];
    bb = P[P[xi + 1] + yi + 1];

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

    int xi = static_cast<int>(fx) & 255;
    int yi = static_cast<int>(fy) & 255;
    int zi = static_cast<int>(fz) & 255;

    x -= fx;
    y -= fy;
    z -= fz;

    float sx = fade(x);
    float sy = fade(y);
    float sz = fade(z);

    const unsigned char* P = m_PTable;

    unsigned char aaa, aba, aab, abb, baa, bba, bab, bbb;
    aaa = P[P[P[xi] + yi] + zi];
    aba = P[P[P[xi] + yi+1] + zi];
    aab = P[P[P[xi] + yi] + zi+1];
    abb = P[P[P[xi] + yi+1] + zi+1];
    baa = P[P[P[xi+1] + yi] + zi];
    bba = P[P[P[xi+1] + yi+1] + zi];
    bab = P[P[P[xi+1] + yi] + zi+1];
    bbb = P[P[P[xi+1] + yi+1] + zi+1];

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

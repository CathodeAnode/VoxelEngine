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
    int xi = static_cast<int>(std::floorf(x)) & 255; // = % 256
    
    x -= std::floorf(x);
    float sx = fade(x);

    unsigned char a, b;
    a = m_PTable[xi];
    b = m_PTable[xi + 1];

    float avg = lerp(
        sx,
        gradient(a, x, 0, 0),
        gradient(a, x - 1, 0, 0)
    );

    return map(avg, -1, 1, 0, 1);
}

float PerlinNoise::Noise2D(float x, float y)
{
    int xi = static_cast<int>(std::floorf(x)) & 255;
    int yi = static_cast<int>(std::floorf(y)) & 255;

    x -= std::floorf(x);
    y -= std::floorf(y);

    float sx = fade(x);
    float sy = fade(y);

    unsigned char aa, ab, ba, bb;
    aa = m_PTable[m_PTable[xi] + yi];
    ab = m_PTable[m_PTable[xi] + yi + 1];
    ba = m_PTable[m_PTable[xi + 1] + yi];
    bb = m_PTable[m_PTable[xi + 1] + yi + 1];

    float avg = lerp(
        sy,
        lerp(
            sx,
            gradient(aa, x, y, 0),
            gradient(ba, x - 1, y, 0)
        ),
        lerp(
            sx,
            gradient(ba, x, y - 1, 0),
            gradient(bb, x - 1, y - 1, 0)
        )
    );

    return map(avg, -1, 1, 0, 1);
}

float PerlinNoise::Noise3D(float x, float y, float z)
{
    int xi = static_cast<int>(std::floorf(x)) & 255;
    int yi = static_cast<int>(std::floorf(y)) & 255;
    int zi = static_cast<int>(std::floorf(z)) & 255;

    x -= std::floorf(x);
    y -= std::floorf(y);
    z -= std::floorf(z);

    float sx = fade(x);
    float sy = fade(y);
    float sz = fade(z);

    unsigned char aaa, aba, aab, abb, baa, bba, bab, bbb;
    aaa = m_PTable[m_PTable[m_PTable[xi] + yi] + zi];
    aba = m_PTable[m_PTable[m_PTable[xi] + yi+1] + zi];
    aab = m_PTable[m_PTable[m_PTable[xi] + yi] + zi+1];
    abb = m_PTable[m_PTable[m_PTable[xi] + yi+1] + zi+1];
    baa = m_PTable[m_PTable[m_PTable[xi+1] + yi] + zi];
    bba = m_PTable[m_PTable[m_PTable[xi+1] + yi+1] + zi];
    bab = m_PTable[m_PTable[m_PTable[xi+1] + yi] + zi+1];
    bbb = m_PTable[m_PTable[m_PTable[xi+1] + yi+1] + zi+1];

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
            sx,
            lerp( // top
                sx,
                gradient(aab, x, y, z - 1),
                gradient(bab, x - 1, y, z - 1)
            ),
            lerp( // bottom
                sx,
                gradient(abb, x, y - 1, z - 1),
                gradient(bbb, x - 1, y - 1, z - 1)
            )
        )
    );

    return map(avg, -1, 1, 0, 1);
}

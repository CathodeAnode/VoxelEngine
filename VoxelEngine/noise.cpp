#include "noise.h"

Noise::Noise(uint32_t seed)
{
    Reseed(seed);
}

void Noise::Reseed(uint32_t seed)
{
    m_Seed = seed;
}

float Noise::PerlinNoise1D(float x)
{
    float xf = std::floor(x);

    uint32_t xi = static_cast<uint32_t>(xf);
    
    x -= xf;
    float sx = fade(x);

    uint32_t a = Hash1D(xi, m_Seed);
    uint32_t b = Hash1D(xi + 1, m_Seed);

    float avg = lerp(
        sx,
        PerlinGradient(a, x, 0, 0),
        PerlinGradient(b, x - 1, 0, 0)
    );

    return avg;
}

float Noise::PerlinNoise2D(float x, float y)
{
    float fx = std::floor(x);
    float fy = std::floor(y);

    uint32_t xi = static_cast<uint32_t>(fx);
    uint32_t yi = static_cast<uint32_t>(fy);

    x -= fx;
    y -= fy;

    float sx = fade(x);
    float sy = fade(y);

    uint32_t aa = Hash2D(xi, yi, m_Seed);
    uint32_t ab = Hash2D(xi, yi + 1, m_Seed);
    uint32_t ba = Hash2D(xi + 1, yi, m_Seed);
    uint32_t bb = Hash2D(xi + 1, yi + 1, m_Seed);

    float avg = lerp(
        sy,
        lerp(
            sx,
            PerlinGradient(aa, x, y, 0),
            PerlinGradient(ba, x - 1, y, 0)
        ),
        lerp(
            sx,
            PerlinGradient(ab, x, y - 1, 0),
            PerlinGradient(bb, x - 1, y - 1, 0)
        )
    );

    return avg;
}

float Noise::PerlinNoise3D(float x, float y, float z)
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
                PerlinGradient(aaa, x, y, z),
                PerlinGradient(baa, x - 1, y, z)
            ),
            lerp( // bottom
                sx,
                PerlinGradient(aba, x, y - 1, z),
                PerlinGradient(bba, x - 1, y - 1, z)
            )
        ),
        lerp( //rear
            sy,
            lerp( // top
                sx,
                PerlinGradient(aab, x, y, z - 1),
                PerlinGradient(bab, x - 1, y, z)
            ),
            lerp( // bottom
                sx,
                PerlinGradient(abb, x, y - 1, z - 1),
                PerlinGradient(bbb, x - 1, y - 1, z - 1)
            )
        )
    );

    return avg;
}

float Noise::SimplexNoise2D(float x, float y)
{
    constexpr float F = 0.366025403785f; // F2 = (sqrt(3) - 1) / 2; Skew factor

    // Skew coordinate
    float skew = (x + y) * F;
    float skewX = x + skew;
    float skewY = y + skew;

    float floorX = std::floor(skewX);
    float floorY = std::floor(skewY);

    float fracX = skewX - floorX;
    float fracY = skewY - floorY;

    int32_t xi = static_cast<int32_t>(floorX);
    int32_t yi = static_cast<int32_t>(floorY);

    int32_t cornersX[3] = {xi, 0, xi + 1};
    int32_t cornersY[3] = {yi, 0, yi + 1};

    // Check what triangle the position is in
    if (fracX > fracY) // (fracX > fracY) -> Lower triangle
    {
        cornersX[1] = xi + 1;
        cornersY[1] = yi;
    }
    else // (fracX < fracY) -> Higher triangle; fracX == fracY -> any (doesn't matter which to choose)
    {
        cornersX[1] = xi;
        cornersY[1] = yi + 1;
    }

    // Compute contribution from each corner
    float result = 0.f;
    for (int i = 0; i < 3; i++)
    {   
        constexpr float G = 0.211324865405f; // G2 = (3 - sqrt(3)) / 6; Unskew factor
        float unskew = (cornersX[i] + cornersY[i]) * G;
        float dx = x - (cornersX[i] - unskew);
        float dy = y - (cornersY[i] - unskew);

        // Distance squared
        float dist2 = dx * dx + dy * dy;

        if (dist2 < 0.5f) 
        {
            const uint32_t hash = Hash2D(cornersX[i], cornersY[i], m_Seed);

            float gradientX, gradientY;
            SimplexGradient2D(hash, gradientX, gradientY);

            float dot = dx * gradientX + dy * gradientY;
            float t = 0.5f - dist2;
            float contribution = t * t * t * t * dot;

            result += contribution; 
        }
        else continue; // Corner does basically 0 contribution, so ignore it
    }

    return result * 70.0f; // Normalize it to be around -1 to 1
}

float Noise::SimplexNoise3D(float x, float y, float z)
{
    constexpr float F = 0.333333333333f; // F3 = 1/3; Skew factor

    // Skew coordinate
    float skew = (x + y + z) * F;
    float skewX = x + skew;
    float skewY = y + skew;
    float skewZ = z + skew;

    float floorX = std::floor(skewX);
    float floorY = std::floor(skewY);
    float floorZ = std::floor(skewZ);

    float fracX = skewX - floorX;
    float fracY = skewY - floorY;
    float fracZ = skewZ - floorZ;

    int32_t xi = static_cast<int32_t>(floorX);
    int32_t yi = static_cast<int32_t>(floorY);
    int32_t zi = static_cast<int32_t>(floorZ);

    int32_t cornersX[4] = {xi, 0, 0, xi + 1};
    int32_t cornersY[4] = {yi, 0, 0, yi + 1};
    int32_t cornersZ[4] = {zi, 0, 0, zi + 1};

    // Check what triangle the position is in
    if (fracX >= fracY && fracY >= fracZ)
    {
        cornersX[1] = xi + 1;
        cornersY[1] = yi;
        cornersZ[1] = zi;

        cornersX[2] = xi + 1;
        cornersY[2] = yi + 1;
        cornersZ[2] = zi;
    }
    else if (fracX >= fracZ && fracZ >= fracY)
    {
        cornersX[1] = xi + 1;
        cornersY[1] = yi;
        cornersZ[1] = zi;

        cornersX[2] = xi + 1;
        cornersY[2] = yi;
        cornersZ[2] = zi + 1;
    }
    else if (fracY >= fracX && fracX >= fracZ)
    {
        cornersX[1] = xi;
        cornersY[1] = yi + 1;
        cornersZ[1] = zi;

        cornersX[2] = xi + 1;
        cornersY[2] = yi + 1;
        cornersZ[2] = zi;
    }
    else if (fracY >= fracZ && fracZ >= fracX)
    {
        cornersX[1] = xi;
        cornersY[1] = yi + 1;
        cornersZ[1] = zi;

        cornersX[2] = xi;
        cornersY[2] = yi + 1;
        cornersZ[2] = zi + 1;
    }
    else if (fracZ >= fracX && fracX >= fracY)
    {
        cornersX[1] = xi;
        cornersY[1] = yi;
        cornersZ[1] = zi + 1;

        cornersX[2] = xi + 1;
        cornersY[2] = yi;
        cornersZ[2] = zi + 1;
    }
    else
    {
        cornersX[1] = xi;
        cornersY[1] = yi;
        cornersZ[1] = zi + 1;

        cornersX[2] = xi;
        cornersY[2] = yi + 1;
        cornersZ[2] = zi + 1;
    }

    // Compute contribution from each corner
    float result = 0.f;
    for (int i = 0; i < 4; i++)
    {
        constexpr float G = 0.166666666667f; // G3 = 1/6; Unskew factor
        float dx = x - (cornersX[i] - (cornersX[i] + cornersY[i] + cornersZ[i]) * G);
        float dy = y - (cornersY[i] - (cornersX[i] + cornersY[i] + cornersZ[i]) * G);
        float dz = z - (cornersZ[i] - (cornersX[i] + cornersY[i] + cornersZ[i]) * G);

        // Distance squared
        float dist2 = dx * dx + dy * dy + dz * dz;

        if (dist2 < 0.6f) 
        {
            const uint32_t hash = Hash3D(cornersX[i], cornersY[i], cornersZ[i], m_Seed);

            float gradientX, gradientY, gradientZ;
            SimplexGradient3D(hash, gradientX, gradientY, gradientZ);

            float dot = dx * gradientX + dy * gradientY + dz * gradientZ;
            float t = 0.6f - dist2;
            float contribution = t * t * t * t * dot;

            result += contribution; 
        }
        else continue; // Corner does basically 0 contribution, so ignore it
    }

    return result * 32.0f; // Normalize it to be around -1 to 1
}
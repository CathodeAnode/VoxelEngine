#version 460

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout (std430, binding = 0) buffer noise {
    float value[];
};

uniform ivec3 origin;
uniform uint seed;
uniform float frequency;
uniform float amplitude;

vec3 Gradient(uvec3 p)
{
    const vec3 gradients[12] = {
        vec3( 1.0,  1.0,  0.0),
        vec3(-1.0,  1.0,  0.0),
        vec3( 1.0, -1.0,  0.0),
        vec3(-1.0, -1.0,  0.0),
        vec3( 1.0,  0.0,  1.0),
        vec3(-1.0,  0.0,  1.0),
        vec3( 1.0,  0.0, -1.0),
        vec3(-1.0,  0.0, -1.0),
        vec3( 0.0,  1.0,  1.0),
        vec3( 0.0, -1.0,  1.0),
        vec3( 0.0,  1.0, -1.0),
        vec3( 0.0, -1.0, -1.0)
    };

    uvec3 seed3 = uvec3(seed);
    uvec3 v = p ^ seed3;
    v = (v << 13u) ^ v ^ seed3;
    v = v * (v.x * 0x9E3779B9u + v.y * 0x85EBCA77u + v.z * 0xC2B2AE3Du + seed);
    v ^= v >> 16u;
    v *= 0x85EBCA77u ^ seed;
    v ^= v >> 13u;
    v *= 0xC2B2AE3Du ^ seed;
    v ^= v >> 16u;
    uint hash = v.x ^ v.y ^ v.z ^ seed;

    return gradients[hash % 12];
}

float SimplexNoise3D(vec3 p)
{
    const float F = 0.333333333333; // F3 = 1/3; Skew factor

    // Skew coordinate
    vec3 skewed = p + (p.x + p.y + p.z) * F;

    vec3 floored = floor(skewed);

    vec3 fractional = skewed - floored;

    ivec3 pi = ivec3(floored);

    ivec3 corners[4] = {
        ivec3(pi.x, pi.y, pi.z),
        ivec3(0), ivec3(0),
        ivec3(pi.x + 1, pi.y + 1, pi.z + 1)
    };

    // Check what triangle the position is in
    if (fractional.x >= fractional.y && fractional.y >= fractional.z)
    {
        corners[1] = ivec3(pi.x + 1, pi.y, pi.z);
        corners[2] = ivec3(pi.x + 1, pi.y + 1, pi.z);
    }
    else if (fractional.x >= fractional.z && fractional.z >= fractional.y)
    {
        corners[1] = ivec3(pi.x + 1, pi.y, pi.z);
        corners[2] = ivec3(pi.x + 1, pi.y, pi.z + 1);
    }
    else if (fractional.y >= fractional.x && fractional.x >= fractional.z)
    {
        corners[1] = ivec3(pi.x, pi.y + 1, pi.z);
        corners[2] = ivec3(pi.x + 1, pi.y + 1, pi.z);
    }
    else if (fractional.y >= fractional.z && fractional.z >= fractional.x)
    {
        corners[1] = ivec3(pi.x, pi.y + 1, pi.z);
        corners[2] = ivec3(pi.x, pi.y + 1, pi.z + 1);
    }
    else if (fractional.z >= fractional.x && fractional.x >= fractional.y)
    {
        corners[1] = ivec3(pi.x, pi.y, pi.z + 1);
        corners[2] = ivec3(pi.x + 1, pi.y, pi.z + 1);
    }
    else
    {
        corners[1] = ivec3(pi.x, pi.y, pi.z + 1);
        corners[2] = ivec3(pi.x, pi.y + 1, pi.z + 1);
    }

    // Compute contribution from each corner
    float result = 0.0;
    for (int i = 0; i < 4; i++)
    {
        const float G = 0.166666666667; // G3 = 1/6; Unskew factor
        vec3 delta = p - (vec3(corners[i]) - (corners[i].x + corners[i].y + corners[i].z) * G);

        // Distance squared
        float dist2 = dot(delta, delta);

        if (dist2 < 0.6) 
        {
            vec3 gradient = Gradient(corners[i]);

            float dot = dot(delta, gradient);
            float t = 0.6 - dist2;
            float contribution = t * t * t * t * dot;

            result += contribution; 
        }
        else continue; // Corner does basically 0 contribution, so ignore it
    }

    return result * 32.0; // Normalize it to be around -1 to 1
}

void main()
{
    uvec3 id = gl_GlobalInvocationID;

    uvec3 p = origin + id;

    uint index = id.x + id.y * 8 + id.z * 64;
    value[index] = SimplexNoise3D(p * frequency) * amplitude;
}
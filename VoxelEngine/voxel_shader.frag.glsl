#version 460 core

out vec4 Fragcolor;
flat in uint color;

uniform vec3 lightDir;   // should be normalized
flat in vec3 normal;


void main() 
{
    float r = float((color >> 24) & 0xFFu) / 255.0;
    float g = float((color >> 16) & 0xFFu) / 255.0;
    float b = float((color >> 8) & 0xFFu) / 255.0;
    float a = float(color & 0xFFu) / 255.0;

    // Basic Lambert shading
    float diffuse = max(dot(normal, -lightDir), 0.7); // 0.2 = ambient term so faces never go fully black

    Fragcolor = vec4(r, g, b, a) * diffuse;

}
#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#include "shader.h"

#include <glm/glm.hpp>

class ComputeShader :
    public Shader
{
public:
    ComputeShader() = default;

    ComputeShader(const char* filepath);

    void Dispatch(unsigned int x, unsigned int y, unsigned int z);

    /// <summary>
    /// bitwise combination of any of GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT, GL_ELEMENT_ARRAY_BARRIER_BIT, GL_UNIFORM_BARRIER_BIT, GL_TEXTURE_FETCH_BARRIER_BIT, 
    /// GL_SHADER_IMAGE_ACCESS_BARRIER_BIT, GL_COMMAND_BARRIER_BIT, GL_PIXEL_BUFFER_BARRIER_BIT, GL_TEXTURE_UPDATE_BARRIER_BIT, GL_BUFFER_UPDATE_BARRIER_BIT, 
    /// GL_FRAMEBUFFER_BARRIER_BIT, GL_TRANSFORM_FEEDBACK_BARRIER_BIT, GL_ATOMIC_COUNTER_BARRIER_BIT, or GL_SHADER_STORAGE_BARRIER_BIT.
    /// OR
    /// bitwise combination of any of GL_ATOMIC_COUNTER_BARRIER_BIT, or GL_FRAMEBUFFER_BARRIER_BIT, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT, GL_SHADER_STORAGE_BARRIER_BIT. 
    /// GL_TEXTURE_FETCH_BARRIER_BIT, or GL_UNIFORM_BARRIER_BIT.
    /// OR
    /// GL_ALL_BARRIER_BITS
    /// </summary>
    void Wait(GLbitfield barriers);

    inline unsigned int GetLocalSizeX() { return m_LocalSize.x; }
    inline unsigned int GetLocalSizeY() { return m_LocalSize.y; }
    inline unsigned int GetLocalSizeZ() { return m_LocalSize.z; }
    inline glm::ivec3 GetLocalSizeGroup() { return m_LocalSize; }

private:
    glm::ivec3 m_LocalSize;

private:
    bool _ParseLocalGroupSize(const std::string& shaderSrc);
};

#endif


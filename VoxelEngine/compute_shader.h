#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#include "shader.h"

#include <glm/fwd.hpp>
#include <regex>

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
    inline bool _ParseLocalGroupSize(const std::string& shaderSrc)
    {
        std::string src;
        src.reserve(shaderSrc.size());
        src = _StripGLSLComments(shaderSrc);


        // Find the layout(...) in; block
        static const std::regex layoutBlockRegex(
            R"(layout\s*\(([^)]*)\)\s*in\s*;)",
            std::regex::icase
        );

        std::smatch blockMatch;
        if (!std::regex_search(src, blockMatch, layoutBlockRegex)) {
            return false;
        }

        const std::string& layoutContents = blockMatch[1].str();

        // Match individual local_size entries
        static const std::regex entryRegex(
            R"(local_size_(x|y|z)\s*=\s*(\d+))",
            std::regex::icase
        );

        for (std::sregex_iterator it(layoutContents.begin(), layoutContents.end(), entryRegex);
            it != std::sregex_iterator();
            ++it)
        {
            char axis = std::tolower((*it)[1].str()[0]);
            int value = std::stoi((*it)[2].str());

            switch (axis)
            {
            case 'x': m_LocalSize.x = value; break;
            case 'y': m_LocalSize.y = value; break;
            case 'z': m_LocalSize.z = value; break;
            }
        }

        return true;
    }
};

#endif


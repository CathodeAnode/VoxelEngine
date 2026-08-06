#include "compute_shader.h"

#include <regex>

#include "profiler.h"

ComputeShader::ComputeShader(const char* filepath)
	: Shader({ {filepath, GL_COMPUTE_SHADER} })
    , m_LocalSize(1)
{
	PROFILE_FUNCTION();

    _ParseLocalGroupSize(_LoadShaderSrc(filepath));
}

void ComputeShader::Dispatch(unsigned int x, unsigned int y, unsigned int z)
{
	assert(x >= 1 && y >= 1 && z >= 1);
	assert(m_Id != 0);
	glDispatchCompute(x, y, z);
}

void ComputeShader::Wait(GLbitfield barriers)
{
	glMemoryBarrier(barriers);
}

void ComputeShader::WaitDispatchCompletion()
{
	GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	assert(sync != 0);

    // Wait indefinitely for GPU completion
	GLenum result = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
	assert(result != GL_WAIT_FAILED);

	glDeleteSync(sync);
}

bool ComputeShader::_ParseLocalGroupSize(const std::string& shaderSrc)
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

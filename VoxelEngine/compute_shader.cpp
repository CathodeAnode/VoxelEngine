#include "compute_shader.h"

ComputeShader::ComputeShader(const char* filepath)
	: Shader({ {filepath, GL_COMPUTE_SHADER} })
    , m_LocalSize(1)
{
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

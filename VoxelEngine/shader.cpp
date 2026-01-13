#include "shader.h"

Shader::Shader(std::initializer_list<ShaderFile> shaders)
{
	LOG_INFO(EngineSystem::RENDERER, "Creating shader program with {} shaders", shaders.size());
	int success;
	char infoLog[512];

	std::vector<GLuint> shaderIDs;
	shaderIDs.reserve(shaders.size());
	for (const auto& shader : shaders)
	{
		LOG_INFO(
			EngineSystem::RENDERER,
			"Compiling shader: path='{}', type={}",
			shader.shaderPath,
			shader.TypeToString()
		);


		shaderIDs.push_back(compileShader(shader.shaderPath, shader.shaderType));
	}

	m_Id = glCreateProgram();

	for (const auto& shaderID : shaderIDs)
	{
		glAttachShader(m_Id, shaderID);
		LOG_TRACE(
			EngineSystem::RENDERER,
			"Attached shader ID {} to program {}",
			shaderID,
			m_Id
		);
	}

	glLinkProgram(m_Id);

	glGetProgramiv(m_Id, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(m_Id, 512, NULL, infoLog);
		LOG_ERROR(
			EngineSystem::RENDERER,
			"Shader program link failed (program ID {}): {}",
			m_Id,
			infoLog
		);
	}

	LOG_INFO(
		EngineSystem::RENDERER,
		"Shader program (ID={}) linked successfully",
		m_Id
	);

	for (const auto& shaderID : shaderIDs)
	{
		glDeleteShader(shaderID);
		LOG_TRACE(
			EngineSystem::RENDERER,
			"Deleted intermediate shader (ID={})",
			shaderID
		);
	}
}

Shader::~Shader()
{
	LOG_INFO(
		EngineSystem::RENDERER,
		"Destroying shader program (ID={})",
		m_Id
	);

	assert(m_Id != 0);
	glDeleteShader(m_Id);
}

void Shader::Use() 
{
	PROFILE_FUNCTION();

	LOG_TRACE(
		EngineSystem::RENDERER,
		"Using shader program (ID={})",
		m_Id
	);
	
	assert(m_Id != 0);

	glUseProgram(m_Id);
}


unsigned int Shader::compileShader(const char* path, int shaderType) 
{
	int success;
	char infoLog[512];

	unsigned int shader;
	shader = glCreateShader(shaderType);

	LOG_TRACE(
		EngineSystem::RENDERER,
		"Created shader object (ID={}) for '{}'",
		shader,
		path
	);

	std::string shaderSrc = loadShaderSrc(path);
	const GLchar* shaderStr = shaderSrc.c_str();
	glShaderSource(shader, 1, &shaderStr, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		LOG_ERROR(
			EngineSystem::RENDERER,
			"Shader compilation failed for '{}': {}",
			path,
			infoLog
		);
	}

	LOG_TRACE(
		EngineSystem::RENDERER,
		"Shader compiled successfully: '{}'",
		path
	);

	return shader;
}

std::string Shader::loadShaderSrc(const char* path) 
{
	if (!std::filesystem::exists(path)) {
		LOG_CRITICAL(
			EngineSystem::RENDERER,
			"Shader file does not exist '{}'",
			path
		);
		return {};
	}

	std::fstream file;
	std::stringstream buf;

	std::string ret = "";

	file.open(path);

	if (file.is_open()) {
		buf << file.rdbuf();
		ret = buf.str();
	}
	else {
		LOG_ERROR(
			EngineSystem::RENDERER,
			"Failed to open shader file '{}'",
			path
		);
	}

	file.close();
	LOG_TRACE(
		EngineSystem::RENDERER,
		"Loaded shader source from '{}'",
		path
	);

	return ret;
}

GLuint Shader::getVarLocation(const char* name) const
{
	GLuint location = glGetUniformLocation(m_Id, name);

	assert(location != -1 && "Could not find variable in shader");

	return location;
}

void Shader::SetMat4(const char* name, const glm::mat4& val, unsigned int count) const
{
	glUniformMatrix4fv(getVarLocation(name), count, GL_FALSE, glm::value_ptr(val));
}

void Shader::SetVec4(const char* name, const glm::vec4& val, unsigned int count) const
{
	glUniform4fv(getVarLocation(name), count, glm::value_ptr(val));
}

void Shader::SetIVec3(const char* name, const glm::ivec3& val, unsigned int count) const
{
	glUniform3iv(getVarLocation(name), count, glm::value_ptr(val));
}

void Shader::SetBool(const char* name, bool value) const
{
	glUniform1i(getVarLocation(name), (int)value);
}

void Shader::SetInt(const char* name, int value) const
{
	glUniform1i(getVarLocation(name), value);
}

void Shader::SetUInt(const char* name, unsigned int value) const
{
	glUniform1ui(getVarLocation(name), value);
}

void Shader::SetFloat(const char* name, float value) const
{
	glUniform1f(getVarLocation(name), value);
}
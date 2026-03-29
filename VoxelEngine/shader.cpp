#include "shader.h"

#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <initializer_list>
#include <vector>
#include <span>
#include <glm/gtc/type_ptr.hpp>

#include "logger.h"
#include "profiler.h"

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


		shaderIDs.push_back(_CompileShader(shader.shaderPath, shader.shaderType));
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
	Cleanup();
}

Shader::Shader(Shader&& other) noexcept 
{
	m_Id = other.m_Id;
	other.m_Id = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept 
{
	if (this != &other) 
	{
		Cleanup();
		m_Id = other.m_Id;
		other.m_Id = 0;
	}
	return *this;
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


unsigned int Shader::_CompileShader(const char* path, int shaderType) 
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

	std::string shaderSrc = _LoadShaderSrc(path);
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

std::string Shader::_LoadShaderSrc(const char* path) 
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

std::string Shader::_StripGLSLComments(const std::string& shaderSrc)
{
	std::string out;
	out.reserve(shaderSrc.size());

	enum class State {
		Normal,
		LineComment,
		BlockComment,
		String,
		Char
	};

	State state = State::Normal;

	for (size_t i = 0; i < shaderSrc.size(); ++i)
	{
		char c = shaderSrc[i];
		char next = (i + 1 < shaderSrc.size()) ? shaderSrc[i + 1] : '\0';

		switch (state)
		{
		case State::Normal:
			if (c == '/' && next == '/') {
				state = State::LineComment;
				++i;
			}
			else if (c == '/' && next == '*') {
				state = State::BlockComment;
				++i;
			}
			else if (c == '"') {
				state = State::String;
				out += c;
			}
			else if (c == '\'') {
				state = State::Char;
				out += c;
			}
			else {
				out += c;
			}
			break;

		case State::LineComment:
			if (c == '\n') {
				state = State::Normal;
				out += c;
			}
			break;

		case State::BlockComment:
			if (c == '*' && next == '/') {
				state = State::Normal;
				++i;
			}
			break;

		case State::String:
			out += c;
			if (c == '\\' && next != '\0') {
				// Escape sequence: copy next char verbatim
				out += next;
				++i;
			}
			else if (c == '"') {
				state = State::Normal;
			}
			break;

		case State::Char:
			out += c;
			if (c == '\\' && next != '\0') {
				out += next;
				++i;
			}
			else if (c == '\'') {
				state = State::Normal;
			}
			break;
		}
	}

	return out;
}

GLuint Shader::GetVarLocation(const char* name) const
{
	GLuint location = glGetUniformLocation(m_Id, name);

	assert(location != -1 && "Could not find variable in shader");

	return location;
}

void Shader::Cleanup()
{
	LOG_INFO(
		EngineSystem::RENDERER,
		"Destroying shader program (ID={})",
		m_Id
	);

	if(m_Id != 0)
		glDeleteShader(m_Id);
}

void Shader::SetMat4(const char* name, const glm::mat4& val) const
{
	glUniformMatrix4fv(GetVarLocation(name), 1, GL_FALSE, glm::value_ptr(val));
}

void Shader::SetVec4(const char* name, const glm::vec4& val) const
{
	glUniform4fv(GetVarLocation(name), 1, glm::value_ptr(val));
}

void Shader::SetVec4Array(const char* name, const glm::vec4* values, unsigned int count) const
{
	glUniform4fv(GetVarLocation(name), count, glm::value_ptr(values[0]));
	//assert(glGetError() == GL_NO_ERROR);
}

void Shader::SetIVec3(const char* name, const glm::ivec3& val) const
{
	glUniform3iv(GetVarLocation(name), 1, glm::value_ptr(val));
}

void Shader::SetBool(const char* name, bool value) const
{
	glUniform1i(GetVarLocation(name), (int)value);
}

void Shader::SetInt(const char* name, int value) const
{
	glUniform1i(GetVarLocation(name), value);
}

void Shader::SetUInt(const char* name, unsigned int value) const
{
	glUniform1ui(GetVarLocation(name), value);
	//assert(glGetError() == GL_NO_ERROR);
}

void Shader::SetFloat(const char* name, float value) const
{
	glUniform1f(GetVarLocation(name), value);
}

void Shader::GetVec4(const char* name, glm::vec4* out, unsigned int count) const
{
	assert(out != nullptr);

	glGetnUniformfv(m_Id, GetVarLocation(name), count, glm::value_ptr(out[0]));
	//assert(glGetError() == GL_NO_ERROR);
}
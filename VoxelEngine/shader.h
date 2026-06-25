#ifndef SHADER_H
#define SHADER_H

#include<glad/glad.h>

#include <glm/fwd.hpp>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <initializer_list>

#include "logger.h"

struct ShaderFile
{
	const char* shaderPath;
	GLenum shaderType; //GL_COMPUTE_SHADER, GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, or GL_FRAGMENT_SHADER

	const char* TypeToString() const
	{
		switch (shaderType)
		{
		case GL_COMPUTE_SHADER:
			return "Compute_Shader";
		case GL_VERTEX_SHADER:
			return "Vertex_Shader";
		case GL_TESS_CONTROL_SHADER:
			return "Tessellation_Control_Shader";
		case GL_TESS_EVALUATION_SHADER:
			return "Tessellation_Evaluation_Shader";
		case GL_GEOMETRY_SHADER:
			return "Geometry_Shader";
		case GL_FRAGMENT_SHADER:
			return "Fragment_Shader";
		default:
			return "Unknown_Shader_Type";
		}
	}
};

class Shader {
public:

	Shader() = default;
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	// constructor reads and compiles shaders into program
	Shader(std::initializer_list<ShaderFile> shaders);
	~Shader();
	Shader(Shader&& other) noexcept;
	Shader& operator=(Shader&& other) noexcept;

	// uses shader
	void Use();

	GLint GetVarLocation(const char* name) const;

	// util uniform functions
	void SetMat4(const char* name, const glm::mat4& value) const;
	void SetVec4(const char* name, const glm::vec4& value) const;
	void SetVec4Array(const char* name, const glm::vec4* values, unsigned int count) const;
	void SetIVec3(const char* name, const glm::ivec3& value) const;
	void SetVec3(const char* name, const glm::vec3& value) const;

	void SetBool(const char* name, bool value) const;
	void SetInt(const char* name, int value) const;
	void SetUInt(const char* name, unsigned int value) const;
	void SetFloat(const char* name, float value) const;
	// Implement setters and getters as needed

	// TODO: implement getter functions uniform vals: getBool, getInt, getFloat
	void GetVec4(const char* name, glm::vec4* out, unsigned int count) const;


	inline GLuint GetID() const { return m_Id; }

protected:
	GLuint m_Id; // program ID

	inline unsigned int _CompileShader(const char* path, int shaderType)
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

	inline std::string _LoadShaderSrc(const char* path)
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


	std::string _StripGLSLComments(const std::string& shaderSrc);
	void Cleanup();
};

#include "compute_shader.h"


#endif
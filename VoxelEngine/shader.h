#ifndef SHADER_H
#define SHADER_H

#include<glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <initializer_list>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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

	Shader() = delete;

	// constructor reads and compiles shaders into program
	Shader(std::initializer_list<ShaderFile> shaders);
	~Shader();

	// delete copy&move functionality for now
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;
	Shader(Shader&&) = delete;
	Shader& operator=(Shader&&) = delete;

	// uses shader
	void Use();

	// util uniform functions
	void SetMat4(const std::string& name, glm::mat4 value) const;
	void SetBool(const std::string& name, bool value) const;
	void SetInt(const std::string& name, int value) const;
	void SetFloat(const std::string& name, float value) const;
	// Implement setters and getters as needed

	// TODO: implement getter functions uniform vals: getBool, getInt, getFloat

	inline GLuint GetID() const { return m_Id; }

private:
	GLuint m_Id; // program ID
	unsigned int compileShader(const char* path, int shaderType);
	std::string loadShaderSrc(const char* path);
};


#endif
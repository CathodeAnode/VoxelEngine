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

struct ShaderFile
{
	const char* shaderPath;
	GLenum shaderType; //GL_COMPUTE_SHADER, GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, or GL_FRAGMENT_SHADER
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

	// TODO: implement getter functions uniform vals: getBool, getInt, getFloat

	inline GLuint GetID() const { return m_Id; }

private:
	GLuint m_Id; // program ID
	unsigned int compileShader(const char* path, int shaderType);
	std::string loadShaderSrc(const char* path);
};


#endif
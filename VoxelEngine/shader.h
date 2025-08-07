#ifndef SHADER_H
#define SHADER_H

#include<glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


class Shader {
public:
	GLuint m_Id; // program ID

	Shader() = default;

	// constructor reads and compiles shaders into program
	Shader(const char* vertexPath, const char* fragmentPath);

	// uses shader
	void use();

	// util uniform functions
	void setMat4(const std::string& m_Name, glm::mat4 value) const;
	void setBool(const std::string& m_Name, bool value) const;
	void setInt(const std::string& m_Name, int value) const;
	void setFloat(const std::string& m_Name, float value) const;

	// TODO: implement getter functions uniform vals: getBool, getInt, getFloat

private:
	unsigned int compileShader(const char* path, int shaderType);
	std::string loadShaderSrc(const char* path);
};


#endif
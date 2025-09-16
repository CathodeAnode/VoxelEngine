#include "shader.h"

//Shader::Shader() : Shader("object.vs", "object.fs") {}


Shader::Shader(std::initializer_list<ShaderFile> shaders)
{
	int success;
	char infoLog[512];

	std::vector<GLuint> shaderIDs;
	shaderIDs.reserve(shaders.size());
	for (const auto& shader : shaders)
	{
		shaderIDs.push_back(compileShader(shader.shaderPath, shader.shaderType));
	}

	m_Id = glCreateProgram();

	for (const auto& shaderID : shaderIDs)
	{
		glAttachShader(m_Id, shaderID);
	}

	glLinkProgram(m_Id);

	glGetProgramiv(m_Id, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(m_Id, 512, NULL, infoLog);
		std::cout << "Error: could not link shader program\n" << infoLog << "\n";
	}

	for (const auto& shaderID : shaderIDs)
	{
		glDeleteShader(shaderID);
	}
}

Shader::~Shader()
{
	glDeleteShader(m_Id);
}

void Shader::Use() 
{
	glUseProgram(m_Id);
}


unsigned int Shader::compileShader(const char* path, int shaderType) 
{
	int success;
	char infoLog[512];

	unsigned int shader;
	shader = glCreateShader(shaderType);
	std::string shaderSrc = loadShaderSrc(path);
	const GLchar* shaderStr = shaderSrc.c_str();
	glShaderSource(shader, 1, &shaderStr, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cout << "Error: could not compile \"" << path << "\" shader\n" << infoLog << "\n";
	}

	return shader;
}

std::string Shader::loadShaderSrc(const char* path) 
{
	std::fstream file;
	std::stringstream buf;

	std::string ret = "";

	file.open(path);

	if (file.is_open()) {
		buf << file.rdbuf();
		ret = buf.str();
	}
	else {
		std::cout << "Error: Unable to open shader file at (" << path << ")\n";
	}

	file.close();
	return ret;
}

void Shader::SetMat4(const std::string& name, glm::mat4 val) const 
{
	glUniformMatrix4fv(glGetUniformLocation(m_Id, name.c_str()), 1, GL_FALSE, glm::value_ptr(val));
}

void Shader::SetBool(const std::string& name, bool value) const
{
	glUniform1i(glGetUniformLocation(m_Id, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value) const
{
	glUniform1i(glGetUniformLocation(m_Id, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const
{
	glUniform1f(glGetUniformLocation(m_Id, name.c_str()), value);
}
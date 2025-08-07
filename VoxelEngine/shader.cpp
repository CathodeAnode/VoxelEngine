#include "shader.h"

//Shader::Shader() : Shader("object.vs", "object.fs") {}

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
	int success;
	char infoLog[512];

	GLuint vertexShader = compileShader(vertexPath, GL_VERTEX_SHADER);
	GLuint fragmentShader = compileShader(fragmentPath, GL_FRAGMENT_SHADER);

	
	m_Id = glCreateProgram();

	glAttachShader(m_Id, vertexShader);
	glAttachShader(m_Id, fragmentShader);
	glLinkProgram(m_Id);

	glGetProgramiv(m_Id, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(m_Id, 512, NULL, infoLog);
		std::cout << "Error: could not link shader program\n" << infoLog << "\n";
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void Shader::use() {
	glUseProgram(m_Id);
}


unsigned int Shader::compileShader(const char* path, int shaderType) {
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

std::string Shader::loadShaderSrc(const char* path) {
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

void Shader::setMat4(const std::string& m_Name, glm::mat4 val) const {
	glUniformMatrix4fv(glGetUniformLocation(m_Id, m_Name.c_str()), 1, GL_FALSE, glm::value_ptr(val));
}

void Shader::setBool(const std::string& m_Name, bool value) const {
	glUniform1i(glGetUniformLocation(m_Id, m_Name.c_str()), (int)value);
}

void Shader::setInt(const std::string& m_Name, int value) const {
	glUniform1i(glGetUniformLocation(m_Id, m_Name.c_str()), value);
}

void Shader::setFloat(const std::string& m_Name, float value) const {
	glUniform1f(glGetUniformLocation(m_Id, m_Name.c_str()), value);
}
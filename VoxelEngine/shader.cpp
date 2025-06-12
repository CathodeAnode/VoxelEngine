#include "shader.h"

//Shader::Shader() : Shader("object.vs", "object.fs") {}

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
	int success;
	char infoLog[512];

	GLuint vertexShader = compileShader(vertexPath, GL_VERTEX_SHADER);
	GLuint fragmentShader = compileShader(fragmentPath, GL_FRAGMENT_SHADER);

	
	id = glCreateProgram();

	glAttachShader(id, vertexShader);
	glAttachShader(id, fragmentShader);
	glLinkProgram(id);

	glGetProgramiv(id, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(id, 512, NULL, infoLog);
		std::cout << "Error: could not link shader program\n" << infoLog << "\n";
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void Shader::use() {
	glUseProgram(id);
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

void Shader::setMat4(const std::string& name, glm::mat4 val) const {
	glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, glm::value_ptr(val));
}

void Shader::setBool(const std::string& name, bool value) const {
	glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value) const {
	glUniform1i(glGetUniformLocation(id, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const {
	glUniform1f(glGetUniformLocation(id, name.c_str()), value);
}
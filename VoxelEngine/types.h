#ifndef TYPES_H
#define TYPES_H

#include <glad/glad.h>

struct VertexAttribute {
	GLuint index;       // Location in shader
	GLint size;         // Number of components (e.g., 3 for vec3)
	GLenum type;        // GL_FLOAT, GL_INT, etc.
	GLboolean normalized;
	size_t offset;      // Offset in bytes
};

#endif

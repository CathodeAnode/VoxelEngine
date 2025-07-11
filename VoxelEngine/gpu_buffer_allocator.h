#ifndef GPU_BUFFER_ALLOCATOR_H
#define GPU_BUFFER_ALLOCATOR_H

#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <vector>
#include <stdexcept>

template<typename T>
class GPUBufferAllocator
{
public:
	GPUBufferAllocator(GLenum bufferType, GLenum usage, const std::vector<T>& data);
	GPUBufferAllocator(GLenum bufferType, GLenum usage, unsigned int size);
	~GPUBufferAllocator();

	void upload(const std::vector<T>& data);
	void append(T data);
	void append(const std::vector<T>& data);
	void insert(T data, unsigned int index);
	void insert(const std::vector<T>& data, unsigned int index);
	void replace(const std::vector<T>& data, unsigned int index, unsigned int oldSize);

	void resize(unsigned int size);

	unsigned int getSize() const;
	GLuint getBufferID() const;


private:
	unsigned int bufferSize;
	unsigned int bufferID;
	unsigned int currentSize;

	GLenum type;



};


#endif




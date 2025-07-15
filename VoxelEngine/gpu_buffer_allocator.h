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
	GPUBufferAllocator(GLenum bufferType, GLenum bufferUsage, const std::vector<T>& data);
	GPUBufferAllocator(GLenum bufferType, GLenum bufferUsage, unsigned int size);
	~GPUBufferAllocator();

	void upload(const std::vector<T>& data);
	void append(T data);
	void append(const std::vector<T>& data);
	void insert(T data, unsigned int index);
	void insert(const std::vector<T>& data, unsigned int index);
	void replace(const std::vector<T>& data, unsigned int index, unsigned int oldSize);

	void resize(unsigned int size);

	inline unsigned int getMaxSize() { return bufferSize; };
	inline unsigned int getSize() const { return currentSize; };
	inline GLuint getBufferID() const { return bufferID; };


private:
	unsigned int bufferSize;
	unsigned int bufferID;
	unsigned int currentSize;

	GLenum type;
	GLenum usage;

	// assumes move is valid in buffer, i.e. endIndex + size < buffersize and startIndex < buffersize
	// does not change currentSize
	void move(unsigned int startIndex, unsigned int endIndex, unsigned int size);



};


#include "gpu_buffer_allocator.tpp"


#endif




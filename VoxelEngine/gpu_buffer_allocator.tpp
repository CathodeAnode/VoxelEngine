#include "gpu_buffer_allocator.h"


template<typename T>
GPUBufferAllocator<T>::GPUBufferAllocator(GLenum bufferType, GLenum usage, const std::vector<T>& data)
	: bufferSize(data.size()), type(bufferType)
{
	glGenBuffers(1, &bufferID);
	glBindBuffer(bufferType, bufferID);
	glBufferData(bufferType, bufferSize * sizeof(T), data, usage);
	glBindBuffer(bufferType, 0);

	currentSize = bufferSize;
}

template<typename T>
GPUBufferAllocator<T>::GPUBufferAllocator(GLenum bufferType, GLenum usage, unsigned int size)
{
	if (bufferType == GL_STATIC_DRAW || bufferType == GL_STATIC_COPY || bufferType == GL_STATIC_READ) {
		std::logic_error("Static buffers must be initialized with data");
	}

	glGenBuffers(1, &bufferID);
	glBindBuffer(bufferType, bufferID);
	glBufferData(bufferType, bufferSize * sizeof(T), nullptr, usage);
	glBindBuffer(bufferType, 0);

	currentSize = 0;
}

template<typename T>
GPUBufferAllocator<T>::~GPUBufferAllocator()
{
	glDeleteBuffers(1, &bufferID);
}

template<typename T>
void GPUBufferAllocator<T>::upload(const std::vector<T>& data)
{
	if (type == GL_STATIC_DRAW || type == GL_STATIC_COPY || type == GL_STATIC_READ) {
		std::logic_error("Static buffers cannot be modifiy");
	}

	if (data.size() > bufferSize) {
		resize(data.size());
	}

	glBindBuffer(bufferType, bufferID);
	glBufferSubData(bufferType, 0, data.size() * sizeof(T), data.data());
	glBindBuffer(bufferType, 0);

	currentSize = data.size();
}

template<typename T>
void GPUBufferAllocator<T>::append(T data)
{
	if (currentSize + 1 > bufferSize) {
		resize(bufferSize * 2 + 1); // grow strategy
	}
	glBindBuffer(bufferType, bufferID);
	glBufferSubData(bufferType, currentSize * sizeof(T), sizeof(T), *data);
	glBindBuffer(bufferType, 0);
	
	currentSize++;
}

template<typename T>
void GPUBufferAllocator<T>::append(const std::vector<T>& data)
{
	if (currentSize + data.size() > bufferSize) {
		resize(bufferSize * 2  + 1);
	}

	glBindBuffer(bufferType, bufferID);
	glBufferSubData(bufferType, currentSize * sizeof(T), data.size() * sizeof(T), data.data());
	glBindBuffer(bufferType, 0);

	currentSize += data.size();
}

template<typename T>
void GPUBufferAllocator<T>::insert(T data, unsigned int index)
{
	if (currentSize + 1 > bufferSize) {
		resize(bufferSize * 2 + 1); // grow strategy
	}

	glBindBuffer(GL_COPY_READ_BUFFER, bufferID);
	glBindBuffer(GL_COPY_WRITE_BUFFER, bufferID);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, index * sizeof(T), (index + 1) * sizeof(T), sizeof(T) * (currentSize - index));
	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);


	glBindBuffer(bufferType, bufferID);
	glBufferSubData(bufferType, index * sizeof(T), sizeof(T), *data);
	glBindBuffer(bufferType, 0);

	currentSize++;
}

template<typename T>
void GPUBufferAllocator<T>::insert(const std::vector<T>& data, unsigned int index)
{
	if (currentSize + data.size() > bufferSize) {
		resize(bufferSize * 2 + 1);
	}

	glBindBuffer(GL_COPY_READ_BUFFER, bufferID);
	glBindBuffer(GL_COPY_WRITE_BUFFER, bufferID);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, index * sizeof(T), (index + data.size()) * sizeof(T), sizeof(T) * (currentSize - index));
	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);


	glBindBuffer(bufferType, bufferID);
	glBufferSubData(bufferType, index * sizeof(T), sizeof(T) * data.size(), data.data());
	glBindBuffer(bufferType, 0);

	currentSize += data.size();
}

template<typename T>
void GPUBufferAllocator<T>::replace(const std::vector<T>& data, unsigned int index, unsigned int oldSize)
{
	if (currentSize + (data.size() - oldSize) > bufferSize) {
		resize(bufferSize * 2 + 1);
	}

	glBindBuffer(GL_COPY_READ_BUFFER, bufferID);
	glBindBuffer(GL_COPY_WRITE_BUFFER, bufferID);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, (index + oldSize) * sizeof(T), (index + data.size()) * sizeof(T), sizeof(T) * (currentSize - index + oldSize));
	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

	glBindBuffer(bufferType, bufferID);
	glBufferSubData(bufferType, index * sizeof(T), sizeof(T) * data.size(), data.data());
	glBindBuffer(bufferType, 0);

	currentSize += (data.size() - oldSize);

}

template<typename T>
void GPUBufferAllocator<T>::resize(unsigned int newSize)
{
	
}

template<typename T>
unsigned int GPUBufferAllocator<T>::getSize() const
{
	return bufferSize;
}

template<typename T>
GLuint GPUBufferAllocator<T>::getBufferID() const
{
	return bufferID;
}
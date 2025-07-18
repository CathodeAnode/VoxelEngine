#ifndef GPU_BUFFER_ALLOCATOR_H
#define GPU_BUFFER_ALLOCATOR_H

#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <vector>
#include <stdexcept>

#include "gpu_buffer_lock.h"

//template<typename T>
//class GPUBufferAllocator
//{
//public:
//	GPUBufferAllocator(GLenum bufferType, GLenum bufferUsage, const std::vector<T>& data);
//	GPUBufferAllocator(GLenum bufferType, GLenum bufferUsage, unsigned int size);
//	~GPUBufferAllocator();
//
//	void upload(const std::vector<T>& data);
//	void append(T data);
//	void append(const std::vector<T>& data);
//	void insert(T data, unsigned int index);
//	void insert(const std::vector<T>& data, unsigned int index);
//	void replace(const std::vector<T>& data, unsigned int index, unsigned int oldSize);
//
//	void resize(unsigned int size);
//
//	inline unsigned int getMaxSize() const { return bufferSize; };
//	inline unsigned int getSize() const { return currentSize; };
//	inline GLuint getBufferID() const { return bufferID; };
//
//
//private:
//	unsigned int bufferSize;
//	unsigned int bufferID;
//	unsigned int currentSize;
//
//	GLenum type;
//	GLenum usage;
//
//	// assumes move is valid in buffer, i.e. endIndex + size < buffersize and startIndex < buffersize
//	// does not change currentSize
//	void move(unsigned int startIndex, unsigned int endIndex, unsigned int size);
//
//
//
//};

enum class BufferStorage
{
    SystemMemory,
    PersistentlyMappedBuffer
};

template<typename Atom>
class GPUBuffer
{
public:
    GPUBuffer(bool _cpuUpdates);
    ~GPUBuffer();

    bool Create(BufferStorage _storage, GLenum _target, GLuint _count, GLbitfield _createFlags, GLbitfield _mapFlags);
    void Destroy();

    void WaitForLockedRange(size_t _lockBegin, size_t _lockLength);
    void LockRange(size_t _lockBegin, size_t _lockLength);

    void BindBuffer();
    void BindBufferBase(GLuint _index);
    void BindBufferRange(GLuint _index, GLsizeiptr _head, GLsizeiptr _count);

    Atom* GetContents() { return bufferContents; };
    GLsizeiptr getSize() const { return sizeAtoms; };

private:
    GPUBufferLockManager lockManager;
    Atom* bufferContents;
    GLuint name;
    GLenum target;
    GLsizeiptr sizeAtoms;


    BufferStorage bufferStorage;
};

template<typename Atom>
class GPUCircularBuffer
{
public:
    GPUCircularBuffer(bool _cpuUpdates = true);

    bool Create(BufferStorage _storage, GLenum _target, GLuint _count, GLbitfield _createFlags, GLbitfield _mapFlags);
    void Destroy();

    Atom* Reserve(GLsizeiptr _count);
    void OnUsageComplete(GLsizeiptr _count);

    void BindBuffer();
    void BindBufferBase(GLuint _index);
    void BindBufferRange(GLuint _index, GLsizeiptr _count);

    GLsizeiptr GetHead() const { return mHead; }
    void* GetHeadOffset() const { return (void*)(mHead * sizeof(Atom)); }
    GLsizeiptr GetSize() const { return buffer.getSize(); }

private:
    GPUBuffer<Atom> buffer;
    GLsizeiptr head;
};

template<typename Atom, typename KeyType>
class GPUPagedBuffer 
{

};

#include "gpu_buffer_allocator.tpp"


#endif

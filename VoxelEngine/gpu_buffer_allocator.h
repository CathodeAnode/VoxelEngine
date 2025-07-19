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

struct Page {
    size_t id;
    size_t index;
    size_t size;
};

template<typename Atom>
class GPUPersistentlyMappedBuffer
{
public:
    GPUPersistentlyMappedBuffer(bool _cpuUpdates);
    ~GPUPersistentlyMappedBuffer();

    bool Create(GLenum _target, GLuint _count);
    void Destroy();

    void WaitForLockedRange(size_t _lockBegin, size_t _lockLength);
    void LockRange(size_t _lockBegin, size_t _lockLength);

    void BindBuffer();
    void BindBufferBase(GLuint _index);
    void BindBufferRange(GLuint _index, GLsizeiptr _head, GLsizeiptr _count);

    Atom* GetContents() { return bufferContents; };
    GLsizeiptr GetSize() const { return sizeAtoms; };
    GLuint GetName() const { return name; };

private:
    GPUBufferLockManager lockManager;
    Atom* bufferContents;
    GLuint name;
    GLenum target;
    GLsizeiptr sizeAtoms;
};

template<typename Atom>
class GPUCircularBuffer
{
public:
    GPUCircularBuffer(bool _cpuUpdates = true);

    bool Create(GLenum _target, GLuint _count);
    void Destroy();

    Atom* Reserve(GLsizeiptr _count);
    void OnUsageComplete(GLsizeiptr _count);

    void BindBuffer();
    void BindBufferBase(GLuint _index);
    void BindBufferRange(GLuint _index, GLsizeiptr _count);

    GLsizeiptr GetHead() const { return head; }
    void* GetHeadOffset() const { return (void*)(head * sizeof(Atom)); }
    GLsizeiptr GetSize() const { return buffer.GetSize(); }

private:
    GPUPersistentlyMappedBuffer<Atom> buffer;
    GLsizeiptr head;
};

template<typename Atom, typename KeyType>
class GPUPagedBuffer 
{
public:
    GPUPagedBuffer();
    ~GPUPagedBuffer();

    bool Create(GLenum _target, GLuint _count);
    void Destroy();

    void AllocatePage(const KeyType& pageKey, size_t size);
    void UploadPageData(const KeyType& pageKey, const std::vector<Atom>& data);
    bool UpdatePage(const KeyType& pageKey, const std::vector<Atom>& data);

    Page GetPageOffset(const KeyType& pageKey) const;

private:
    // Page information stored for each key (the key type is now generic)
    std::unordered_map<KeyType, Page> pageTable;

    GLuint name;
    GLenum target;

    size_t atomCount;
    size_t pageCount;
    size_t maxCount;

    void move(size_t srcIndex, size_t dstIndex, size_t length);

};

#include "gpu_buffer_allocator.tpp"


#endif



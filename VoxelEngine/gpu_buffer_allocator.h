#ifndef GPU_BUFFER_ALLOCATOR_H
#define GPU_BUFFER_ALLOCATOR_H

#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <vector>
#include <stack>
#include <stdexcept>
#include <cassert>

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
//	// assumes move is valid in m_Buffer, i.e. endIndex + size < buffersize and startIndex < buffersize
//	// does not change currentSize
//	void move(unsigned int startIndex, unsigned int endIndex, unsigned int size);
//
//
//
//};

struct Page 
{
    unsigned int index;
    size_t size;

    bool IsNull() {
        return index == 0 && size == 0;
    }
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

    Atom* GetContents() { return m_BufferContents; };
    GLsizeiptr GetSize() const { return m_CountAtoms; };
    GLuint GetName() const { return m_Name; };

private:
    GPUBufferLockManager m_LockManager;
    Atom* m_BufferContents;
    GLuint m_Name;
    GLenum m_Target;
    GLsizeiptr m_CountAtoms;
};

template<typename Atom>
class GPUCircularBuffer
{
public:
    GPUCircularBuffer(bool _cpuUpdates = true);

    bool Create(GLenum _target, GLuint _count);
    void Destroy();

    Atom* Reserve(GLsizeiptr _count);
    GLsizeiptr OnUsageComplete(GLsizeiptr _count);

    void BindBuffer();
    void BindBufferBase(GLuint _index);
    void BindBufferHeadRange(GLuint _index, GLsizeiptr _count);
    void BindBufferRange(GLuint _index, GLsizeiptr _offset, GLsizeiptr _count);

    GLsizeiptr GetHead() const { return m_Head; }
    void* GetHeadOffset() const { return (void*)(m_Head * sizeof(Atom)); }
    GLsizeiptr GetSize() const { return m_Buffer.GetSize(); }

private:
    GPUPersistentlyMappedBuffer<Atom> m_Buffer;
    GLsizeiptr m_Head = 0;
};

template<typename Atom>
class GPUPagedBuffer 
{
public:
    GPUPagedBuffer();
    ~GPUPagedBuffer();

    bool Create(GLenum _target, GLuint _count) noexcept;
    void Destroy() noexcept;

    size_t UploadPageData(const std::vector<Atom>& data) noexcept;
    bool UpdatePage(const size_t& pageId, const std::vector<Atom>& data) noexcept;

    Page GetPageOffset(const size_t& pageId) noexcept;
    size_t GetCurrentSize() const { return m_AtomCount; };
    size_t GetMaxSize() const { return m_MaxAtomCount; };
    size_t GetPageSize() const { return m_PageTable.size(); };
    size_t GetName() const { return m_Name; };


private:
    const int m_KInitialPageTableCapacity = 1000;
    std::vector<Page> m_PageTable;

    GLuint m_Name;
    GLenum m_Target;

    size_t m_AtomCount;
    size_t m_MaxAtomCount;

    void move(size_t srcIndex, size_t dstIndex, size_t length);


};

template<typename Atom, typename ObjectID>
class GPUFixedPagedBuffer
{
public:
    GPUFixedPagedBuffer(bool cpuUpdates = true);

    bool Create(GLenum m_Target, size_t pageSize, size_t pageCount) noexcept;
    void Destroy() noexcept;

    bool AllocatePages(const ObjectID& obj, unsigned int pages);
    bool PushBackToObject(const ObjectID& obj, const Atom& data);
    bool DeallocateObject(const ObjectID& obj);

    std::vector<GPUBufferRange> GetObjectBufferRanges(const ObjectID& obj);

private:
    struct PageRange {
        unsigned int start;
        unsigned int end;

        bool TrimStart(unsigned int n)
        {
            start += n;
            return start <= end; // return if range is still valid
        }

        inline unsigned int GetSize() const { return end - start; };
    };

    struct GPUObjectAllocation
    {
        std::vector<PageRange> pagesAllocated;
        size_t countOnLastPage;
    };

    GPUPersistentlyMappedBuffer<Atom> m_Buffer;
    std::vector<PageRange> m_FreePages;
    std::unordered_map<ObjectID, GPUObjectAllocation> m_ObjectPages; // map obj id => allocated pages, count of elements

    size_t m_PageSize;
};

#include "gpu_buffer_allocator.tpp"


#endif

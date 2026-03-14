#ifndef GPU_BUFFER_ALLOCATOR_H
#define GPU_BUFFER_ALLOCATOR_H

#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <vector>
#include <stdexcept>
#include <cassert>

#include "gpu_buffer_lock.h"
#include "helpers.h"
#include "logger.h"
#include "profiler.h"

struct Page 
{
    size_t index;
    size_t size;

    bool IsNull() {
        return index == 0 && size == 0;
    }
};

enum class BufferAccess : uint8_t {
    WriteOnly,
    ReadWrite,
    ReadOnly
};

namespace GPUAllocatorsUtils
{
    constexpr const char* ToString(BufferAccess access) noexcept
    {
        switch (access)
        {
        case BufferAccess::WriteOnly: return "WriteOnly";
        case BufferAccess::ReadWrite: return "ReadWrite";
        case BufferAccess::ReadOnly:  return "ReadOnly";
        }
        return "Unknown";
    }

    constexpr const char* ToString(GLenum target)
    {
        switch (target)
        {
        case GL_ARRAY_BUFFER:
            return "GL_ARRAY_BUFFER";
        case GL_ATOMIC_COUNTER_BUFFER:
            return "GL_ATOMIC_COUNTER_BUFFER";
        case GL_COPY_READ_BUFFER:
            return "GL_COPY_READ_BUFFER";
        case GL_COPY_WRITE_BUFFER:
            return "GL_COPY_WRITE_BUFFER";
        case GL_DISPATCH_INDIRECT_BUFFER:
            return "GL_DISPATCH_INDIRECT_BUFFER";
        case GL_DRAW_INDIRECT_BUFFER:
            return "GL_DRAW_INDIRECT_BUFFER";
        case GL_ELEMENT_ARRAY_BUFFER:
            return "GL_ELEMENT_ARRAY_BUFFER";
        case GL_PIXEL_PACK_BUFFER:
            return "GL_PIXEL_PACK_BUFFER";
        case GL_PIXEL_UNPACK_BUFFER:
            return "GL_PIXEL_UNPACK_BUFFER";
        case GL_QUERY_BUFFER:
            return "GL_QUERY_BUFFER";
        case GL_SHADER_STORAGE_BUFFER:
            return "GL_SHADER_STORAGE_BUFFER";
        case GL_TEXTURE_BUFFER:
            return "GL_TEXTURE_BUFFER";
        case GL_TRANSFORM_FEEDBACK_BUFFER:
            return "GL_TRANSFORM_FEEDBACK_BUFFER";
        case GL_UNIFORM_BUFFER:
            return "GL_UNIFORM_BUFFER";
        default:
            return "UNKNOWN_GL_BUFFER_TARGET";
        }
    }
}



template<typename Atom, IBufferLockManager LockManager = GPUBufferLockManager>
class GPUPersistentlyMappedBuffer
{
public:
    GPUPersistentlyMappedBuffer(bool _cpuUpdates);
    ~GPUPersistentlyMappedBuffer();

    bool Create(GLenum _target, GLuint _count, BufferAccess access=BufferAccess::WriteOnly);
    void Destroy();

    void WaitForLockedRange(size_t _lockBegin, size_t _lockLength);
    void LockRange(size_t _lockBegin, size_t _lockLength);

    void BindBuffer();
    void BindBufferBase(GLuint _index);
    void BindBufferRange(GLuint _index, size_t _head, size_t _count);

    inline Atom* GetContents() { return m_BufferContents; }
    inline const Atom* GetContents() const { return m_BufferContents; }
    inline size_t GetSize() const { return m_CountAtoms; }
    inline GLuint GetName() const { return m_Name; }

private:
    LockManager m_LockManager;
    Atom* m_BufferContents;
    GLuint m_Name;
    GLenum m_Target;
    uint32_t m_CountAtoms;
};

template<typename Atom, IBufferLockManager LockManager = GPUBufferLockManager>
class GPUCircularBuffer
{
public:
    GPUCircularBuffer(bool _cpuUpdates = true);

    bool Create(GLenum _target, GLuint _count, BufferAccess access = BufferAccess::WriteOnly);
    void Destroy();

    Atom* Reserve(size_t _count);
    Atom* ReserveRange(size_t start, size_t count);
    void OnUsageComplete(size_t _count);

    void BindBuffer();
    void BindBufferBase(GLuint _index);
    void BindBufferHeadRange(GLuint _index, size_t _count);
    void BindBufferRange(GLuint _index, size_t _offset, size_t _count);

    inline size_t GetHead() const { return m_Head; }
    inline void* GetHeadOffset() const { return (void*)(m_Head * sizeof(Atom)); }
    inline size_t GetSize() const { return m_Buffer.GetSize(); }
    inline GLuint GetName() const { return m_Buffer.GetName(); }

private:
    GPUPersistentlyMappedBuffer<Atom, LockManager> m_Buffer;
    size_t m_Head = 0;
};

template<typename Atom, IBufferLockManager LockManager = GPUBufferLockManager>
class GPUOrphanBuffer
{
public:
    GPUOrphanBuffer(bool _cpuUpdates = true);

    bool Create(GLenum target, GLuint countPerBuffer, uint8_t numOfBuffers, BufferAccess access = BufferAccess::WriteOnly);
    void Destroy();

    void AdvanceHead();
    void AdvanceTail();

    void BindHeadBuffer(GLuint index);
    void BindHeadBufferRange(GLuint index, size_t count);

    void BindTailBuffer(GLuint index);
    void BindTailBufferRange(GLuint index, size_t count);

    inline size_t GetHead() const { return m_CircularBuffer.GetHead(); }
    inline void* GetHeadOffset() const { return m_CircularBuffer.GetHeadOffset(); }
    inline Atom* GetHeadContents() { return m_CircularBuffer.Reserve(m_CountPerBuffer); }

    inline size_t GetTail() const { return m_Tail; }
    inline void* GetTailOffset() const { return (void*)(m_Tail * sizeof(Atom)); }
    inline Atom* GetTailContents() { return m_CircularBuffer.ReserveRange(m_Tail, m_CountPerBuffer); }

    inline size_t GetSize() const { return m_CountPerBuffer; }
    inline GLuint GetName() const { return m_CircularBuffer.GetName(); }

private:
    GPUCircularBuffer<Atom, LockManager> m_CircularBuffer;
    size_t m_Tail = 0;
    uint32_t m_CountPerBuffer;
};

template<typename Atom>
class [[deprecated("Unmaintained")]] GPUPagedBuffer
{
public:
    GPUPagedBuffer();
    ~GPUPagedBuffer();

    bool Create(GLenum _target, GLuint _count) noexcept;
    void Destroy() noexcept;

    size_t UploadPageData(const std::vector<Atom>& data) noexcept;
    bool UpdatePage(const size_t& pageId, const std::vector<Atom>& data) noexcept;

    Page GetPageOffset(const size_t& pageId) noexcept;
    size_t GetCurrentSize() const { return m_AtomCount; }
    size_t GetMaxSize() const { return m_MaxAtomCount; }
    size_t GetPageSize() const { return m_PageTable.size(); }
    GLuint GetName() const { return m_Name; }


private:
    const int m_KInitialPageTableCapacity = 1000;
    std::vector<Page> m_PageTable;

    GLuint m_Name;
    GLenum m_Target;

    size_t m_AtomCount;
    size_t m_MaxAtomCount;

    void move(size_t srcIndex, size_t dstIndex, size_t length);
};

#include "gpu_buffer_allocator.tpp"


#endif

#ifndef GPU_BUFFER_ALLOCATOR_H
#define GPU_BUFFER_ALLOCATOR_H

#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <vector>
#include <stdexcept>
#include <cassert>
#include <concepts>
#include <cstddef>

#include "gpu_buffer_lock.h"
#include "helpers.h"
#include "logger.h"
#include "profiler.h"

enum class BufferAccess : uint8_t 
{
    WriteOnly,
    ReadWrite,
    ReadOnly
};

enum class ThreadMode : uint8_t
{
    SingleThreaded,
    LockFree
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

    constexpr const char* ToString(ThreadMode mode) noexcept
    {
        switch (mode)
        {
        case ThreadMode::SingleThreaded: return "SingleThreaded";
        case ThreadMode::LockFree: return "LockFree";
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

    inline GLint GetOffsetAlignment(GLenum target)
    {
        GLint alignment = 0;

        switch (target)
        {
        case GL_UNIFORM_BUFFER:
            glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
            return alignment;

        case GL_SHADER_STORAGE_BUFFER:
            glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &alignment);
            return alignment;

        case GL_TEXTURE_BUFFER:
            glGetIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &alignment);
            return alignment;

        default:
            return 1;
        }
    }
}

template<typename T>
concept GPUSafeStruct = std::is_trivially_copyable_v<T>
    && std::is_standard_layout_v<T>
    && !std::is_polymorphic_v<T>
    && !std::is_reference_v<T>
    && !std::is_pointer_v<T>;



template<GPUSafeStruct Atom, IBufferLockManager LockManager = GPUBufferLockManager>
class GPUPersistentlyMappedBuffer
{
public:
    GPUPersistentlyMappedBuffer(bool _cpuUpdates);
    ~GPUPersistentlyMappedBuffer();

    Atom& operator[](size_t index);
    const Atom& operator[](size_t index) const;

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

template<GPUSafeStruct Atom, IBufferLockManager LockManager = GPUBufferLockManager>
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


template<GPUSafeStruct Atom, size_t FRAME_COUNT>
class GPUOrphanBuffer
{
public:
    static_assert(FRAME_COUNT > 2, "GPUOrphanBuffer frame count must be atleast 3");

    GPUOrphanBuffer(bool _cpuUpdates = true);

    bool Create(GLenum target, GLuint countPerBuffer, BufferAccess access = BufferAccess::WriteOnly);
    void Destroy();

    void Commit();
    void BindPreviousFrame(GLuint index);
    void BindCurrentFrame(GLuint index);

    inline Atom* GetCurrentContents()
    {
        return &m_Buffer.GetContents()[_GetOffset(m_CurrentFrame)];
    }

    inline const Atom* GetPreviousContents() const
    {
        const size_t index = _GetOffset((m_CurrentFrame + FRAME_COUNT - 1) % FRAME_COUNT);
        return &m_Buffer.GetContents()[index];
    }

    size_t GetPrevFrameOffset() const;

    inline size_t GetSize() const { return m_FrameSize; }
    inline GLuint GetName() const { return m_Buffer.GetName(); }
    inline void* GetPreviousFrameOffset() const { return (void*)(_GetOffset((m_CurrentFrame + FRAME_COUNT - 1) % FRAME_COUNT) * sizeof(Atom)); }
    inline size_t GetPreviousFrameByteOffset() const { return _GetOffset((m_CurrentFrame + FRAME_COUNT - 1) % FRAME_COUNT) * sizeof(Atom); }

private:
    inline size_t _GetOffset(size_t frame) const
    {
        return frame * (m_FrameSize);
    }

private:
    GPUPersistentlyMappedBuffer<Atom, NullBufferLockManager> m_Buffer;
    size_t m_CurrentFrame = 0;
    uint32_t m_FrameSize;
};

//template<typename Atom>
//class [[deprecated("Unmaintained")]] GPUPagedBuffer
//{
//public:
//    GPUPagedBuffer();
//    ~GPUPagedBuffer();
//
//    bool Create(GLenum _target, GLuint _count) noexcept;
//    void Destroy() noexcept;
//
//    size_t UploadPageData(const std::vector<Atom>& data) noexcept;
//    bool UpdatePage(const size_t& pageId, const std::vector<Atom>& data) noexcept;
//
//    Page GetPageOffset(const size_t& pageId) noexcept;
//    size_t GetCurrentSize() const { return m_AtomCount; }
//    size_t GetMaxSize() const { return m_MaxAtomCount; }
//    size_t GetPageSize() const { return m_PageTable.size(); }
//    GLuint GetName() const { return m_Name; }
//
//
//private:
//    const int m_KInitialPageTableCapacity = 1000;
//    std::vector<Page> m_PageTable;
//
//    GLuint m_Name;
//    GLenum m_Target;
//
//    size_t m_AtomCount;
//    size_t m_MaxAtomCount;
//
//    void move(size_t srcIndex, size_t dstIndex, size_t length);
//};

template<GPUSafeStruct Atom, ThreadMode Mode>
class GPUPagedBuffer
{
public:
    using Page = uint16_t;

    GPUPagedBuffer(bool cpuUpdates = true);
    ~GPUPagedBuffer();

    Atom* operator[](Page pageNum);
    const Atom* operator[](Page pageNum) const;

    bool Create(GLenum target, size_t pageSize, uint16_t pageCount) noexcept;
    void Destroy() noexcept;

    void ReservePage(Page pageNum);
    void FreePage(Page pageNum);
    [[nodiscard]] bool ReserveFirstAvaliblePages(unsigned int n, std::vector<uint32_t>& outPages);

    void BindBuffer();

    size_t GetPageSize() const noexcept { return m_PageSize; }
    size_t GetPageCount() const noexcept { return m_RawBuffer.GetSize() / m_PageSize; }
    inline GLuint GetName() const { return m_RawBuffer.GetName(); }

    bool IsPageReserved(Page pageNum) const noexcept;

private:
    using WordType = std::conditional_t<Mode == ThreadMode::LockFree, std::atomic<uint64_t>, uint64_t>;

private:

    GPUPersistentlyMappedBuffer<Atom, NullBufferLockManager> m_RawBuffer;
    WordType* m_FreePages;
    size_t m_PageSize;

    inline static constexpr size_t WORD_BITS = 64;
};

#include "gpu_buffer_allocator.tpp"


#endif

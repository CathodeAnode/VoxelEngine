#ifndef GPU_BUFFER_ALLOCATOR_H
#define GPU_BUFFER_ALLOCATOR_H

#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <list>
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

template<typename Atom>
class GPUOrphanBuffer
{
public:
    GPUOrphanBuffer(bool _cpuUpdates = true);

    bool Create(GLenum target, GLuint countPerBuffer, uint8_t numOfBuffers, BufferAccess access = BufferAccess::WriteOnly);
    void Destroy();

    void AdvanceHead();
    void AdvanceTail();

    void BindHeadBuffer();
    void BindHeadBufferRange(size_t count);

    void BindTailBuffer();
    void BindTailBufferRange(size_t count);

    inline size_t GetHead() const { return m_CircularBuffer.GetHead(); }
    inline void* GetHeadOffset() const { return m_CircularBuffer.GetHeadOffset(); }
    inline Atom* GetHeadContents() { return m_CircularBuffer.Reserve(m_CountPerBuffer); }

    inline size_t GetTail() const { return m_Tail; }
    inline void* GetTailOffset() const { return (void*)(m_Tail * sizeof(Atom)); }
    inline Atom* GetTailContents() const { return m_CircularBuffer.ReserveRange(m_Tail, m_CountPerBuffer); }

    inline size_t GetSize() const { return m_CountPerBuffer; }
    inline GLuint GetName() const { return m_CircularBuffer.GetName(); }

private:
    GPUCircularBuffer<Atom, NullBufferLockManager> m_CircularBuffer;
    size_t m_Tail = 0;
    uint32_t m_CountPerBuffer;
};

template<typename Atom>
class [[deprecated("Sht way too inefficient man")]] GPUPagedBuffer
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

template<typename Atom, typename ObjectID>
class GPUPagedLRUCache
{
public:
    GPUPagedLRUCache(bool cpuUpdates = true);
    ~GPUPagedLRUCache();

    bool Create(GLenum target, size_t pageSize, size_t pageCount) noexcept;
    void Destroy() noexcept;

    void AllocatePages(const ObjectID& obj, unsigned int pages);
    void PushBackToObject(const ObjectID& obj, const Atom& data);
    //TODO: make emplace back function for Atom&& (r-value)

    void MoveObject(const ObjectID& src, const ObjectID& dst);
    void Swap(const ObjectID& obj1, const ObjectID& obj2);

    void DeallocateObject(const ObjectID& obj);
    void ClearObject(const ObjectID& obj);
    std::vector<GPUBufferRange> GetObjectBufferRanges(const ObjectID& obj);

    inline bool Has(ObjectID obj) const { return m_ObjectMapping.contains(obj); }
    inline GLuint GetName() const { return m_Buffer.GetName(); }
 
private:
    using ByteType = uint8_t;
    inline static constexpr size_t BYTE_BITS = 8;
    inline static constexpr unsigned int BYTE_TYPE_SIZE = BYTE_BITS * sizeof(ByteType);

    struct ObjectAllocationData
    {
        std::vector<unsigned int> pages;
        std::list<ObjectID>::iterator lruIterator;
        size_t count;

        inline unsigned int GetSize() const noexcept
        {
            return pages.size();
        }

        inline void PushBackPages(std::vector<unsigned int>&& p) noexcept
        {
            pages.insert(pages.end(),
                std::make_move_iterator(p.begin()),
                std::make_move_iterator(p.end()));
        }
    };

private:
    GPUPersistentlyMappedBuffer<Atom, NullBufferLockManager> m_Buffer;
    ByteType* m_FreePages;
    std::list<ObjectID> m_ObjectAccessHistory;
    std::unordered_map<ObjectID, ObjectAllocationData> m_ObjectMapping; // map obj id => allocated pages, count of elements

    size_t m_PageSize;

private:
    [[nodiscard]] inline bool _ReserveFirstFreePages(unsigned int n, std::vector<unsigned int>& pagesReserved)
    {
        assert(m_FreePages != nullptr);

        const size_t pageCount = m_Buffer.GetSize() / m_PageSize;
        const size_t freePagesArrSize = ceil(static_cast<double>(pageCount) / BYTE_TYPE_SIZE);

        for (size_t index = 0; index < freePagesArrSize; index++)
        {
            ByteType pagesStatus = m_FreePages[index];

            while (pagesStatus != 0)
            {
                unsigned long consecutiveReservedPages = GetTrailingZeros(pagesStatus);
                pagesStatus >>= consecutiveReservedPages;
                unsigned long consecutiveFreePages = GetTrailingOnes(pagesStatus);
                unsigned long pagesToConsume = std::min(static_cast<unsigned long>(n), consecutiveFreePages);

                ByteType consumeMask = ~((1 << pagesToConsume) - 1) << consecutiveReservedPages;
                m_FreePages[index] &= consumeMask;
                n -= pagesToConsume;

                for (unsigned long i = 0; i < pagesToConsume; i++)
                {
                    unsigned long pageID = index * BYTE_TYPE_SIZE + consecutiveReservedPages + i;
                    pagesReserved.push_back(pageID);
                }

                if (n == 0)
                    return true;
            }

        }

        if (n < 0)
        {
            LOG_ERROR(EngineSystem::GPU_BUFFER, "[GPUPagedLRUCache|{}] Something went wrong: Allocated more pages than needed"
                , m_Buffer.GetName());
        }

        return false;
    }

    inline void _ReservePages(const std::vector<unsigned int>& pages)
    {
        assert(m_FreePages != nullptr);

        for (const auto& page : pages)
        {
            const size_t byteIdx = page / BYTE_TYPE_SIZE;
            const size_t bitIdx = page % BYTE_TYPE_SIZE;
            m_FreePages[byteIdx] &= ~(1 << bitIdx); // Set bit to 0 => reserved
        }
    }

   inline void _FreePages(const std::vector<unsigned int>& pages)
    {
        assert(m_FreePages != nullptr);
#ifndef NDEBUG
        const size_t pageCount = m_Buffer.GetSize() / m_PageSize;
        const size_t arrSize = ceil(static_cast<double>(pageCount) / BYTE_TYPE_SIZE);
        if (pageCount % BYTE_TYPE_SIZE > 0)
        {
            const uint8_t ghostPages = BYTE_TYPE_SIZE - pageCount % BYTE_TYPE_SIZE;
            for (const auto& page : pages)
            {
                assert(page < (arrSize * BYTE_TYPE_SIZE) - ghostPages, "Not allowed to free ghost pages.");
            }
        }
#endif

        for (const auto& page : pages)
        {
            const size_t byteIdx = page / BYTE_TYPE_SIZE;
            const size_t bitIdx = page % BYTE_TYPE_SIZE;
            m_FreePages[byteIdx] |= (1 << bitIdx); // Set bit to 1 => free
        }

    }

    [[nodiscard]] inline bool _EvictLRUAndReserve(unsigned int n, std::vector<unsigned int>& reservedPages)
    {
        assert(!m_ObjectAccessHistory.empty());

        ObjectID objToEvict = m_ObjectAccessHistory.back();
        m_ObjectAccessHistory.pop_back();
        ObjectAllocationData& objData = m_ObjectMapping[objToEvict];

        _FreePages(objData.pages);

        const int pagesToFree = objData.GetSize() - n;
        size_t numPagesToMove = std::min(static_cast<size_t>(n), objData.pages.size());

        LOG_DEBUG(EngineSystem::GPU_BUFFER,
            "[GPUPagedLRUCache|{}] Evicting object {} from cache (pages_freed={}, pages_reserved={})"
            , m_Buffer.GetName()
            , objToEvict
            , objData.GetSize()
            , numPagesToMove);

        reservedPages.insert(reservedPages.end(),
            std::make_move_iterator(objData.pages.begin()),
            std::make_move_iterator(objData.pages.begin() + numPagesToMove));

        m_ObjectMapping.erase(objToEvict);

        return pagesToFree < 0;
    }

    inline void _MarkRecentlyUsed(const ObjectID& obj)
    {
        assert(m_ObjectMapping.contains(obj));

        ObjectAllocationData& objData = m_ObjectMapping[obj];
        m_ObjectAccessHistory.splice(m_ObjectAccessHistory.begin(), m_ObjectAccessHistory, objData.lruIterator);
        objData.lruIterator = m_ObjectAccessHistory.begin();
    }

};

#include "gpu_buffer_allocator.tpp"


#endif

#ifndef GPU_BUFFER_ALLOCATOR_H
#define GPU_BUFFER_ALLOCATOR_H

#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <vector>
#include <unordered_set>
#include <list>
#include <stdexcept>
#include <cassert>

#include "gpu_buffer_lock.h"
#include "helpers.h"

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
    size_t index;
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
    size_t GetCurrentSize() const { return m_AtomCount; };
    size_t GetMaxSize() const { return m_MaxAtomCount; };
    size_t GetPageSize() const { return m_PageTable.size(); };
    GLuint GetName() const { return m_Name; };


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

    bool Create(GLenum m_Target, size_t pageSize, size_t pageCount) noexcept;
    void Destroy() noexcept;

    void AllocatePages(const ObjectID& obj, unsigned int pages);
    void PushBackToObject(const ObjectID& obj, const Atom& data);
    void MoveObject(const ObjectID& src, const ObjectID& dst);
    void DeallocateObject(const ObjectID& obj);

    std::vector<GPUBufferRange> GetObjectBufferRanges(const ObjectID& obj) const;

private:
    using ByteType = uint8_t;
    constexpr size_t BYTE_BITS = 8;
    constexpr unsigned int BYTE_TYPE_SIZE = BYTE_BITS * sizeof(ByteType);

    struct ObjectAllocationData
    {
        std::vector<unsigned int> pages;
        unsigned int count;
        std::list<ObjectID>::iterator lruIterator;

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
    GPUPersistentlyMappedBuffer<Atom> m_Buffer;
    ByteType* m_FreePages;
    std::list<ObjectID> m_ObjectAccessHistory;
    std::unordered_map<ObjectID, ObjectAllocationData> m_ObjectMapping; // map obj id => allocated pages, count of elements

    size_t m_PageSize;

private:
    bool ReserveFirstFreePages(unsigned int n, std::vector<unsigned int>& pagesReserved)
    {
        assert(m_FreePages != nullptr);

        const size_t pageCount = m_Buffer.GetSize() / m_PageSize;
        const size_t freePagesArrSize = ceil(pageCount / BYTE_TYPE_SIZE);

        for (size_t index = 0; index < freePagesArrSize && n > 0; index++)
        {
            ByteType pagesStatus = m_FreePages[index];

            while (pagesStatus != 0 && n > 0)
            {
                unsigned long reservedPages = GetTrailingZeros(pagesStatus);
                pagesStatus >>= reservedPages;
                unsigned long freePages = GetTrailingOnes(pagesStatus);
                unsigned long pagesToConsume = std::min(n, freePages);

                    ByteType consumeMask = ~((1 << pagesToConsume) - 1) << reservedPages;
                m_FreePages[index] ^= consumeMask;
                n -= pagesToConsume;

                for (unsigned long i = 0; i < pagesToConsume; i++)
                {
                    unsigned long pageID = index * BYTE_TYPE_SIZE + reservedPages + i;
                    pagesReserved.push_back(pageID);
                }
            }

        }

        return n <= 0;
    }

    void ReservePages(const std::vector<unsigned int>& pages)
    {
        assert(m_FreePages != nullptr);

        for (const auto& page : pages)
        {
            const size_t byteIdx = page / BYTE_TYPE_SIZE;
            const size_t bitIdx = page % BYTE_TYPE_SIZE;
            m_FreePages[byteIdx] &= ~(1 << bitIdx); // Set bit to 0 => reserved
        }
    }

    void FreePages(const std::vector<unsigned int>& pages)
    {
        assert(m_FreePages != nullptr);
#ifndef NDEBUG
        const size_t pageCount = m_Buffer.GetSize() / m_PageSize;
        const size_t arrSize = ceil(pageCount / BYTE_TYPE_SIZE);
        if (pageCount % BYTE_TYPE_SIZE > 0)
        {
            const uint8_t ghostPages = BYTE_TYPE_SIZE - pageCount % BYTE_TYPE_SIZE;
            for (const auto& page : pages)
            {
                assert(page < arrSize * BYTE_TYPE_SIZE - ghostPages);
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

    bool EvictLRUAndReserve(unsigned int n, std::vector<unsigned int>& reservedPages)
    {
        ObjectID objToEvict = m_ObjectAccessHistory.back();
        m_ObjectAccessHistory.pop_back();
        ObjectAllocationData& objData = m_ObjectMapping[objToEvict];

        const int pagesToFree = objData.GetSize() - n;


        if (pagesToFree > 0)
        {
            // If we have more pages than needed, free the tail portion
            pagesToBeFreed.insert(pagesToBeFreed.begin(),
                objData.pages.begin() + n, objData.pages.end());
        }

        // Free the pages
        FreePages(pagesToBeFreed);

        reservedPages.insert(reservedPages.end(),
            std::make_move_iterator(objData.pages.begin()),
            std::make_move_iterator(objData.pages.begin() + std::min(n, objData.pages.size()));

        m_ObjectMapping.erase(objToEvict);

        return pagesToFree >= 0;
    }

    void UpdateObjectLRU(const ObjectID& obj)
    {
        assert(m_ObjectMapping.contains(obj));

        ObjectAllocationData& objData = m_ObjectMapping[obj];
        m_ObjectAccessHistory.splice(m_ObjectAccessHistory.begin(), m_ObjectAccessHistory, objData.lruIterator);
        objData.lruIterator = m_ObjectAccessHistory.begin();
    }

};

#include "gpu_buffer_allocator.tpp"


#endif

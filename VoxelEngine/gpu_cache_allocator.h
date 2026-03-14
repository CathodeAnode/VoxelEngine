#ifndef GPU_CACHE_ALLOCATOR_H
#define GPU_CACHE_ALLOCATOR_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <cassert>

#include "gpu_buffer_allocator.h"
#include "gpu_buffer_lock.h"

template<typename ObjectID, typename Atom>
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

#include "gpu_cache_allocator.tpp"

#endif
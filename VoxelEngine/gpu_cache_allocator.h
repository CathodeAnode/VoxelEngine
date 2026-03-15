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
    inline GLuint GetName() const { return m_PagedBuffer.GetName(); }

private:
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
    GPUPagedBuffer<Atom, ThreadMode::SingleThreaded> m_PagedBuffer;
    std::list<ObjectID> m_ObjectAccessHistory;
    std::unordered_map<ObjectID, ObjectAllocationData> m_ObjectMapping; // map obj id => allocated pages, count of elements

private:
    [[nodiscard]] inline bool _EvictLRUAndReserve(unsigned int n, std::vector<unsigned int>& reservedPages)
    {
        assert(!m_ObjectAccessHistory.empty());

        ObjectID objToEvict = m_ObjectAccessHistory.back();
        m_ObjectAccessHistory.pop_back();
        ObjectAllocationData& objData = m_ObjectMapping[objToEvict];

        for (const auto& page : objData.pages)
        {
            m_PagedBuffer.FreePage(page);
        }

        const int pagesToFree = objData.GetSize() - n;
        size_t numPagesToMove = std::min(static_cast<size_t>(n), objData.pages.size());

        LOG_DEBUG(EngineSystem::GPU_BUFFER,
            "[GPUPagedLRUCache|{}] Evicting object {} from cache (pages_freed={}, pages_reserved={})"
            , m_PagedBuffer.GetName()
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
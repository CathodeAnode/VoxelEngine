#ifndef GPU_CACHE_ALLOCATOR_H
#define GPU_CACHE_ALLOCATOR_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <cassert>

#include "gpu_buffer_allocator.h"
#include "gpu_hashmap_allocator.h"
#include "gpu_buffer_lock.h"
#include "gpu_cache_policy.h"

template<typename ObjectID, typename Atom, EvictionPolicy<ObjectID> Policy>
class GPUPagedCache
{
public:
    GPUPagedCache(bool cpuUpdates = true);
    ~GPUPagedCache();

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
    struct ObjectAllocation
    {
        std::vector<unsigned int> pages; // TODO: change to fix array and make constructor of GPUPagedCache determine max size
        size_t elementCount;

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

    Policy m_Policy;
    GPUPagedBuffer <Atom, ThreadMode::LockFree> m_PagedBuffer;
    GPULockFreeHashMap<ObjectID, ObjectAllocation> m_ObjectPages; // TODO rename class to GPUHashMap and choose thread mode through template param

private:
    [[nodiscard]] inline bool _EvictLRUAndReserve(unsigned int n, std::vector<unsigned int>& reservedPages)
    {
        assert(!m_ObjectAccessHistory.empty());

        ObjectID objToEvict = m_ObjectAccessHistory.back();
        m_ObjectAccessHistory.pop_back();

        auto it = m_ObjectMapping.find(objToEvict);
        assert(it != m_ObjectMapping.end());

        ObjectAllocationData& objData = it->second;

        size_t numPagesToMove = std::min(static_cast<size_t>(n), objData.pages.size());

        LOG_DEBUG(EngineSystem::GPU_BUFFER,
            "[GPUPagedLRUCache|{}] Evicting object {} from cache (pages_freed={}, pages_reserved={})",
            m_PagedBuffer.GetName(),
            objToEvict,
            objData.pages.size() - numPagesToMove,
            numPagesToMove);

        reservedPages.insert(reservedPages.end(),
            objData.pages.begin(),
            objData.pages.begin() + numPagesToMove);

        for (size_t i = numPagesToMove; i < objData.pages.size(); ++i)
        {
            m_PagedBuffer.FreePage(objData.pages[i]);
        }

        m_ObjectMapping.erase(it);

        return numPagesToMove < n;
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
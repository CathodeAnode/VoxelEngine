#ifndef GPU_CACHE_ALLOCATOR_H
#define GPU_CACHE_ALLOCATOR_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

#include "gpu_buffer_allocator.h"
#include "gpu_hashmap_allocator.h"
#include "gpu_buffer_lock.h"
#include "gpu_cache_policy.h"


// TODO: pass thread mode & gpu-visiblity in template params
template<typename TObjectID, typename TAtom, template<typename> typename Policy>
    requires EvictionPolicy<Policy<TObjectID>, TObjectID>
class GPUPagedCache
{
public:
    using ObjectID = TObjectID;
    using Atom = TAtom;

public:
    struct ObjectAllocation;
    // TODO pass max objects to constructor
    GPUPagedCache(bool cpuUpdates = true);
    ~GPUPagedCache();
    GPUPagedCache(const GPUPagedCache&) = delete;
    GPUPagedCache& operator=(const GPUPagedCache&) = delete;

    bool Create(GLenum target, size_t pageSize, uint32_t pageCount) noexcept;
    void Destroy() noexcept;

    ObjectAllocation AllocatePages(const ObjectID& obj, uint32_t pageCount);
    void AllocateObject(const ObjectID& obj, const Atom* data, size_t count);
    void PushBackToObject(const ObjectID& obj, const Atom& data);
    //TODO: make emplace back function for Atom&& (r-value)

    void MoveObject(const ObjectID& src, const ObjectID& dst);
    void Swap(const ObjectID& obj1, const ObjectID& obj2);

    void DeallocateObject(const ObjectID& obj);
    void ClearObject(const ObjectID& obj);
    std::vector<GPUBufferRange> GetObjectBufferRanges(const ObjectID& obj);

    void BindCacheData();
    void BindCacheLookup(GLint hashMapLocation, GLint nodesLocation, GLint policyLocation);

    inline bool Has(ObjectID obj) const { return m_ObjectPages.Contains(obj); }
    inline GLuint GetName() const { return m_PagedBuffer.GetName(); }

private:
    template<typename Cache>
    friend class GPUPagedCacheInspector; // for validation, debugging, and unit testing

    struct ObjectAllocation
    {
        uint32_t totalElementCount = 0;
        uint32_t startPage = PageNode::NULL_PAGE;
        uint32_t endPage = PageNode::NULL_PAGE;

        Policy<ObjectID>::Handle policyHandle;

        inline unsigned int GetPageCount(unsigned int pageSize) const noexcept
        {
            assert(pageSize > 0);
            return ((totalElementCount + pageSize - 1) / pageSize) + (totalElementCount == 0 && startPage != PageNode::NULL_PAGE);
        }

        inline bool IsEmpty() const noexcept
        {
            return startPage == PageNode::NULL_PAGE;
        }
    };

    struct PageNode
    {
        static inline constexpr uint32_t NULL_PAGE = std::numeric_limits<uint32_t>::max();

        uint32_t next;
    };

    struct SplitChain
    {
        uint32_t takeStart;
        uint32_t takeEnd;
        uint32_t remainingStart;
    };

private:
    Policy<ObjectID> m_Policy;
    GPUPagedBuffer<Atom, ThreadMode::LockFree> m_PagedBuffer;
    GPUPersistentlyMappedBuffer<PageNode> m_PageNodes;
    GPUHashMap<ObjectID, ObjectAllocation, ThreadMode::LockFree> m_ObjectPages;

private:
    void _FreeChain(uint32_t startPage);
    void _BuildPageChain(ObjectAllocation& alloc, const std::vector<uint32_t>& pages);
    void _AppendPages(ObjectAllocation& alloc, const std::vector<uint32_t>& pages);
    std::unordered_set<uint32_t> _CollectPages(const ObjectAllocation& alloc) const;

    [[nodiscard]] uint32_t _TryReservePages(const ObjectID& obj, uint32_t pageCount, ObjectAllocation& outAlloc);
    [[nodiscard]] uint32_t _EvictAndTakePages(const ObjectID& obj, ObjectAllocation& targetAlloc, uint32_t pagesNeeded);
    SplitChain _SplitVictimChain(const ObjectAllocation& victim, uint32_t pagesToTake);
    void _AttachPages(const ObjectID& obj, ObjectAllocation& target, uint32_t start, uint32_t end);
    uint32_t _CountAllocatedPages(const ObjectAllocation& alloc) const;

};

#include "gpu_cache_allocator.tpp"

#endif
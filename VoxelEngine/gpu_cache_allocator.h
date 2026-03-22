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

template<typename ObjectID, typename Atom, template<typename> typename Policy>
    requires EvictionPolicy<Policy<ObjectID>, ObjectID>
class GPUPagedCache
{
public:
    GPUPagedCache(bool cpuUpdates = true);
    ~GPUPagedCache();

    bool Create(GLenum target, size_t pageSize, uint16_t pageCount) noexcept;
    void Destroy() noexcept;

    void AllocatePages(const ObjectID& obj, uint16_t pageCount);
    void PushBackToObject(const ObjectID& obj, const Atom& data);
    //TODO: make emplace back function for Atom&& (r-value)

    void MoveObject(const ObjectID& src, const ObjectID& dst);
    void Swap(const ObjectID& obj1, const ObjectID& obj2);

    void DeallocateObject(const ObjectID& obj);
    void ClearObject(const ObjectID& obj);
    std::vector<GPUBufferRange> GetObjectBufferRanges(const ObjectID& obj);

    inline bool Has(ObjectID obj) const { return m_ObjectPages.contains(obj); }
    inline GLuint GetName() const { return m_PagedBuffer.GetName(); }

private:
    struct ObjectAllocation
    {
        unsigned int totalElementCount = 0;
        uint16_t startPage = PageNode::NULL_PAGE;
        uint16_t endPage = PageNode::NULL_PAGE;

        inline unsigned int GetPageCount(unsigned int pageSize) const noexcept
        {
            return (totalElementCount + pageSize - 1) / pageSize;
        }

        inline bool IsEmpty() const noexcept
        {
            return startPage == PageNode::NULL_PAGE;
        }
    };

    struct PageNode
    {
        static inline constexpr uint16_t NULL_PAGE = std::numeric_limits<uint16_t>::max();

        uint16_t next;
    };

    struct SplitChain
    {
        uint16_t takeStart;
        uint16_t takeEnd;
        uint16_t remainingStart;
    };

    Policy<ObjectID> m_Policy;
    GPUPagedBuffer<Atom, ThreadMode::LockFree> m_PagedBuffer;
    GPUPersistentlyMappedBuffer<PageNode> m_PageNodes;
    std::unordered_map<ObjectID, ObjectAllocation> m_ObjectPages; // TODO rename class to GPUHashMap and choose thread mode through template param

private:
    void _FreeObject(const ObjectID& obj);
    void _FreeChain(uint16_t startPage);
    void _BuildPageChain(ObjectAllocation& alloc, const std::vector<uint16_t>& pages);
    void _AppendPages(ObjectAllocation& alloc, const std::vector<uint16_t>& pages);
    std::unordered_set<uint16_t> _CollectPages(const ObjectAllocation& alloc);

    [[nodiscard]] bool _TryReservePages(const ObjectID& obj, uint16_t pageCount);
    void _EvictAndTakePages(ObjectAllocation& targetAlloc, uint16_t requiredPages);
    SplitChain _SplitVictimChain(ObjectAllocation& victim, uint16_t pagesToTake);
    void _AttachPages(ObjectAllocation& target, uint16_t start, uint16_t end);


};

#include "gpu_cache_allocator.tpp"

#endif
#ifndef GPU_CACHE_INSPECTOR
#define GPU_CACHE_INSPECTOR

#include <vector>

template<typename Cache>
class GPUPagedCacheInspector
{
public:
    using ObjectID = typename Cache::ObjectID;
    using Atom = typename Cache::Atom;
    using ObjectAllocation = typename Cache::ObjectAllocation;
    using Page = uint32_t;

public:
    static std::vector<Atom> ReadObjectData(const Cache& cache, const ObjectID& id)
    {
        std::vector<Atom> result;

        typename Cache::ObjectAllocation alloc;

        if (!cache.m_ObjectPages.Find(id, alloc))
            return result;

        if (alloc.totalElementCount == 0 || alloc.IsEmpty())
            return result;

        result.reserve(alloc.totalElementCount);

        const size_t pageSize = cache.m_PagedBuffer.GetPageSize();

        size_t remaining = alloc.totalElementCount;

        uint32_t current = alloc.startPage;

        while (current != Cache::PageNode::NULL_PAGE && remaining > 0)
        {
            const Atom* pageData = cache.m_PagedBuffer[current];

            const size_t count = std::min(pageSize, remaining);

            result.insert(result.end(), pageData, pageData + count);

            remaining -= count;

            current = cache.m_PageNodes[current].next;
        }

        return result;
    }

    static unsigned int GetFreePagesCount(const Cache& cache)
    {
        unsigned int freePages = 0;

        const size_t pageCount = cache.m_PagedBuffer.GetPageCount();

        for (Page i = 0; i < pageCount; ++i)
        {
            if (!cache.m_PagedBuffer.IsPageReserved(i))
            {
                freePages++;
            }
        }

        return freePages;
    }

    static uint32_t CountAllocatedPages(const Cache& cache, const ObjectAllocation& alloc)
    {
        return cache._CountAllocatedPages(alloc);
    }
};

#endif
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

        std::unordered_set<uint32_t> pages = cache._CollectPages(alloc);

        result.reserve(alloc.totalElementCount);

        const size_t pageSize = cache.m_PagedBuffer.GetPageSize();

        size_t remaining = alloc.totalElementCount;

        for (uint32_t page : pages)
        {
            const Atom* pageData = cache.m_PagedBuffer[page];
            const size_t count = std::min(pageSize, remaining);

            result.insert(result.end(),pageData, pageData + count);

            remaining -= count;

            if (remaining == 0)
                break;
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
};

#endif
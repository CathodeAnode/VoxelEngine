#ifndef GPU_CACHE_ALLOCATOR_TPP
#define GPU_CACHE_ALLOCATOR_TPP

#include "gpu_cache_allocator.h"

template<typename ObjectID, typename Atom>
GPUPagedLRUCache<ObjectID, Atom>::GPUPagedLRUCache(bool cpuUpdates)
	: m_Buffer(cpuUpdates)
{
	PROFILE_FUNCTION();
}

template<typename ObjectID, typename Atom>
GPUPagedLRUCache<ObjectID, Atom>::~GPUPagedLRUCache()
{
	Destroy();
}

template<typename ObjectID, typename Atom>
bool GPUPagedLRUCache<ObjectID, Atom>::Create(GLenum target, size_t pageSize, size_t pageCount) noexcept
{
	PROFILE_FUNCTION();

	assert(pageSize > 0 && pageCount > 0);

	bool result = m_Buffer.Create(target, pageSize * pageCount);

	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUPagedLRUCache|{}] Created (pages={}, countPerPage={})",
		m_Buffer.GetName(), pageSize, pageCount);

	LOG_DEBUG(EngineSystem::GPU_BUFFER,
		"[GPUPagedLRUCache|{}] reserved {:.2f} Mb",
		m_Buffer.GetName(),
		((sizeof(Atom) * pageCount * pageSize) / 1000000.0f));

	m_PageSize = pageSize;

	// calculate the number of elements needed for m_FreePages
	const size_t arrSize = std::ceil(static_cast<double>(pageCount) / BYTE_TYPE_SIZE);

	m_FreePages = new ByteType[arrSize];
	std::fill(m_FreePages, m_FreePages + arrSize, std::numeric_limits<ByteType>::max());

	// reserve ghost pages in data struct
	if (pageCount % BYTE_TYPE_SIZE > 0)
	{
		const uint8_t lastElemUsedPages = pageCount % BYTE_TYPE_SIZE;
		const ByteType ghostPagesMask = ~((1u << lastElemUsedPages) - 1);
		m_FreePages[arrSize - 1] ^= ghostPagesMask;
	}

	return result;
}

template<typename ObjectID, typename Atom>
void GPUPagedLRUCache<ObjectID, Atom>::Destroy() noexcept
{
	PROFILE_FUNCTION();

	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUPagedLRUCache|{}] Destroyed",
		m_Buffer.GetName());

	m_PageSize = 0;
	delete[] m_FreePages;
	m_ObjectMapping.clear();
	m_ObjectAccessHistory.clear();

	m_Buffer.Destroy();
}

template<typename ObjectID, typename Atom>
void GPUPagedLRUCache<ObjectID, Atom>::AllocatePages(const ObjectID& obj, unsigned int pages)
{
	assert(pages > 0 && m_FreePages != nullptr);

	std::vector<unsigned int> allocatedPages;
	allocatedPages.reserve(pages);


	// Not enough free pages found
	bool sufficientPagesFound = _ReserveFirstFreePages(pages, allocatedPages);
	while (!sufficientPagesFound)
	{
		sufficientPagesFound = _EvictLRUAndReserve(pages - allocatedPages.size(), allocatedPages);
	}

	// Add to object mapping
	if (m_ObjectMapping.contains(obj))
	{
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUPagedLRUCache|{}] allocating {} pages for exisiting object {}",
			m_Buffer.GetName(),
			pages,
			obj);

		ObjectAllocationData& objAlloc = m_ObjectMapping[obj];
		objAlloc.PushBackPages(std::move(allocatedPages));
		_MarkRecentlyUsed(obj);
	}
	else
	{
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUPagedLRUCache|{}] allocating {} pages for new object {}",
			m_Buffer.GetName(),
			pages,
			obj);

		ObjectAllocationData& objAlloc = m_ObjectMapping[obj];
		objAlloc.PushBackPages(std::move(allocatedPages));
		m_ObjectAccessHistory.push_front(obj);
		objAlloc.lruIterator = m_ObjectAccessHistory.begin();

	}
}

template<typename ObjectID, typename Atom>
void GPUPagedLRUCache<ObjectID, Atom>::PushBackToObject(const ObjectID& obj, const Atom& data)
{
	// Check if object has any pages
	if (!m_ObjectMapping.contains(obj))
	{
		AllocatePages(obj, 1);
	}

	ObjectAllocationData& objAlloc = m_ObjectMapping[obj];

	// Calculate the current page index for next insertion
	const unsigned int pageIndex = objAlloc.count / m_PageSize;
	const unsigned int pageElemOffset = objAlloc.count % m_PageSize;

	// Allocate new page if needed
	if (pageIndex >= objAlloc.GetSize())
	{
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUPagedLRUCache|{}] Push back overflow. Allocating page for object {}.",
			m_Buffer.GetName(),
			obj);
		AllocatePages(obj, 1);
	}

	Atom* bufferHead = m_Buffer.GetContents();
	const unsigned int targetPage = objAlloc.pages[pageIndex];

	bufferHead[targetPage * m_PageSize + pageElemOffset] = data;
	objAlloc.count++;
	_MarkRecentlyUsed(obj);
}

template<typename ObjectID, typename Atom>
void GPUPagedLRUCache<ObjectID, Atom>::MoveObject(const ObjectID& src, const ObjectID& dst)
{
	_FreePages(m_ObjectMapping[src].pages);
	m_ObjectMapping[dst] = std::move(m_ObjectMapping[src]);
	m_ObjectMapping.erase(src);
	m_ObjectAccessHistory.erase(src);
	_MarkRecentlyUsed(dst);
}

template<typename ObjectID, typename Atom>
void GPUPagedLRUCache<ObjectID, Atom>::Swap(const ObjectID& obj1, const ObjectID& obj2)
{
	std::swap(m_ObjectMapping[obj1], m_ObjectMapping[obj2]);
	_MarkRecentlyUsed(obj1);
	_MarkRecentlyUsed(obj2);
}

template<typename ObjectID, typename Atom>
void GPUPagedLRUCache<ObjectID, Atom>::DeallocateObject(const ObjectID& obj)
{
	if (!m_ObjectMapping.contains(obj))
		return;

	ObjectAllocationData& alloc = m_ObjectMapping[obj];

	_FreePages(alloc.pages);
	m_ObjectAccessHistory.erase(alloc.lruIterator);
	m_ObjectMapping.erase(obj);
}

template<typename ObjectID, typename Atom>
void GPUPagedLRUCache<ObjectID, Atom>::ClearObject(const ObjectID& obj)
{
	if (!m_ObjectMapping.contains(obj))
		return;

	ObjectAllocationData& alloc = m_ObjectMapping[obj];

	alloc.count = 0;
	// LRU policy is not updated for object
}

template<typename ObjectID, typename Atom>
std::vector<GPUBufferRange> GPUPagedLRUCache<ObjectID, Atom>::GetObjectBufferRanges(const ObjectID& obj)
{
	assert(m_ObjectMapping.contains(obj));
	std::vector<GPUBufferRange> result;

	const ObjectAllocationData& alloc = m_ObjectMapping.at(obj);
	const unsigned int writePageIdx = (alloc.count - 1) / m_PageSize;
	const unsigned int writePage = alloc.pages[writePageIdx];

	std::unordered_set<unsigned int> pagesSet;
	pagesSet.insert(alloc.pages.begin(), alloc.pages.begin() + writePageIdx + 1);

	for (const auto& page : pagesSet)
	{
		// if start of buffer range
		if (pagesSet.find(page - 1) == pagesSet.end())
		{
			const unsigned int startPage = page;
			unsigned int endPage = page;

			while (pagesSet.find(endPage + 1) != pagesSet.end())
				endPage++;


			size_t pagesRange = (endPage - startPage);
			size_t usedInLastPage = alloc.count % m_PageSize;
			usedInLastPage = (usedInLastPage == 0 ? m_PageSize : usedInLastPage);

			size_t length = pagesRange * m_PageSize + usedInLastPage;
			result.emplace_back(startPage * m_PageSize, length);

		}
	}

	_MarkRecentlyUsed(obj);

	return result;
}


#endif
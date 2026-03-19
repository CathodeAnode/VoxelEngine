#ifndef GPU_CACHE_ALLOCATOR_TPP
#define GPU_CACHE_ALLOCATOR_TPP

#include "gpu_cache_allocator.h"

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
GPUPagedCache<ObjectID, Atom, Policy>::GPUPagedCache(bool cpuUpdates)
	: m_PagedBuffer(cpuUpdates)
{
	PROFILE_FUNCTION();
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
GPUPagedCache<ObjectID, Atom, Policy>::~GPUPagedCache()
{
	Destroy();
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
bool GPUPagedCache<ObjectID, Atom, Policy>::Create(GLenum target, size_t pageSize, size_t pageCount) noexcept
{
	PROFILE_FUNCTION();

	bool result = m_PagedBuffer.Create(target, pageSize, pageCount);

	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUPagedCache|{}] Created (pages={}, countPerPage={})",
		m_PagedBuffer.GetName(), pageSize, pageCount);

	LOG_DEBUG(EngineSystem::GPU_BUFFER,
		"[GPUPagedCache|{}] reserved {:.2f} Mb",
		m_PagedBuffer.GetName(),
		((sizeof(Atom) * pageCount * pageSize) / 1000000.0f));

	return result;
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::Destroy() noexcept
{
	PROFILE_FUNCTION();

	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUPagedCache|{}] Destroyed",
		m_PagedBuffer.GetName());

	m_ObjectPages.clear();

	m_PagedBuffer.Destroy();
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::AllocatePages(const ObjectID& obj, unsigned int pages)
{
	std::vector<unsigned int> allocatedPages;
	allocatedPages.reserve(pages);


	// Not enough free pages found
	bool sufficientPagesFound = m_PagedBuffer.ReserveFirstAvaliblePages(pages, allocatedPages);
	while (!sufficientPagesFound)
	{
		sufficientPagesFound = _EvictLRUAndReserve(pages - allocatedPages.size(), allocatedPages);
	}

	// Add to object mapping
	if (m_ObjectPages.contains(obj))
	{
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] allocating {} pages for exisiting object {}",
			m_PagedBuffer.GetName(),
			pages,
			obj);

		ObjectAllocation& objAlloc = m_ObjectPages[obj];
		objAlloc.PushBackPages(std::move(allocatedPages));
		m_Policy.OnAccess(obj);
	}
	else
	{
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] allocating {} pages for new object {}",
			m_PagedBuffer.GetName(),
			pages,
			obj);

		ObjectAllocation& objAlloc = m_ObjectPages[obj];
		objAlloc.PushBackPages(std::move(allocatedPages));
		m_Policy.OnInsert(obj);
	}
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::PushBackToObject(const ObjectID& obj, const Atom& data)
{
	// Check if object has any pages
	if (!m_ObjectPages.contains(obj))
	{
		AllocatePages(obj, 1);
	}

	ObjectAllocation& objAlloc = m_ObjectPages[obj];

	// Calculate the current page index for next insertion
	const unsigned int pageIndex = objAlloc.count / m_PagedBuffer.GetPageSize();
	const unsigned int pageElemOffset = objAlloc.count % m_PagedBuffer.GetPageSize();

	// Allocate new page if needed
	if (pageIndex >= objAlloc.GetSize())
	{
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] Push back overflow. Allocating page for object {}.",
			m_PagedBuffer.GetName(),
			obj);
		AllocatePages(obj, 1);
	}

	const unsigned int targetPage = objAlloc.pages[pageIndex];

	m_PagedBuffer[targetPage][pageElemOffset] = data;
	objAlloc.count++;
	m_Policy.OnAccess(obj);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::MoveObject(const ObjectID& src, const ObjectID& dst)
{
	for (const auto& page : m_ObjectPages[src].pages)
	{
		m_PagedBuffer.FreePage(page);
	}
	m_ObjectPages[dst] = std::move(m_ObjectPages[src]);
	m_ObjectPages.erase(src);
	m_Policy.OnRemove(src);
	m_Policy.OnAccess(dst);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::Swap(const ObjectID& obj1, const ObjectID& obj2)
{
	std::swap(m_ObjectPages[obj1], m_ObjectPages[obj2]);
	m_Policy.OnAccess(obj1);
	m_Policy.OnAccess(obj2);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::DeallocateObject(const ObjectID& obj)
{
	if (!m_ObjectPages.contains(obj))
		return;

	ObjectAllocation& alloc = m_ObjectPages[obj];

	for (const auto& page : alloc.pages)
	{
		m_PagedBuffer.FreePage(page);
	}
	m_Policy.OnRemove(obj);
	m_ObjectPages.erase(obj);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::ClearObject(const ObjectID& obj)
{
	if (!m_ObjectPages.contains(obj))
		return;

	ObjectAllocation& alloc = m_ObjectPages[obj];

	alloc.count = 0;
	m_Policy.OnAccess(obj);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
std::vector<GPUBufferRange> GPUPagedCache<ObjectID, Atom, Policy>::GetObjectBufferRanges(const ObjectID& obj)
{
	assert(m_ObjectPages.contains(obj));
	std::vector<GPUBufferRange> result;

	const ObjectAllocation& alloc = m_ObjectPages.at(obj);
	const unsigned int writePageIdx = (alloc.count - 1) / m_PagedBuffer.GetPageSize();
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
			size_t usedInLastPage = alloc.count % m_PagedBuffer.GetPageSize();
			usedInLastPage = (usedInLastPage == 0 ? m_PagedBuffer.GetPageSize() : usedInLastPage);

			size_t length = pagesRange * m_PagedBuffer.GetPageSize() + usedInLastPage;
			result.emplace_back(startPage * m_PagedBuffer.GetPageSize(), length);

		}
	}

	m_Policy.OnAccess(obj);

	return result;
}


#endif
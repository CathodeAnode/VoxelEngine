#ifndef GPU_CACHE_ALLOCATOR_TPP
#define GPU_CACHE_ALLOCATOR_TPP

#include "gpu_cache_allocator.h"

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
GPUPagedCache<ObjectID, Atom, Policy>::GPUPagedCache(bool cpuUpdates)
	: m_PagedBuffer(cpuUpdates)
	, m_PageNodes(cpuUpdates)
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
bool GPUPagedCache<ObjectID, Atom, Policy>::Create(GLenum target, size_t pageSize, uint16_t pageCount) noexcept
{
	PROFILE_FUNCTION();

	assert(pageCount <= PageNode::NULL_PAGE - 1);

	GLint maxSizeBytes = 0;
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxSizeBytes);

	if (pageCount * sizeof(PageNode) >= maxSizeBytes)
	{
		LOG_CRITICAL(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] Page node buffer exceeds UBO max size (pageCount={}, maxPageCount={}, required={:.2f} KB, limit={:.2f} KB)",
			m_PagedBuffer.GetName(),
			pageCount,
			maxSizeBytes / sizeof(PageNode),
			(pageCount * sizeof(PageNode)) / 1024.0f,
			maxSizeBytes / 1024.0f);
		return false;
	}

	bool result = m_PagedBuffer.Create(target, pageSize, pageCount);
	result |= m_PageNodes.Create(GL_UNIFORM_BUFFER, pageCount, BufferAccess::ReadWrite);

	PageNode initValue{ .next = PageNode::NULL_PAGE };
	std::fill_n(m_PageNodes.GetContents(), pageCount, initValue);

	//TODO: add other gpu buffers to calculation
	//TODO: change GPUPagedCache to GPUPaged{type}Cache
	LOG_INFO(EngineSystem::GPU_BUFFER,
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
	m_PageNodes.Destroy();
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::AllocatePages(const ObjectID& obj, uint16_t pageCount)
{
	PROFILE_FUNCTION();

	const unsigned int pageSize = m_PagedBuffer.GetPageSize();

	bool fullyReserved = _TryReservePages(obj, pageCount);

	ObjectAllocation& targetAlloc = m_ObjectPages[obj];
	while (!fullyReserved)
	{
		_EvictAndTakePages(targetAlloc, pageCount);
		fullyReserved = (targetAlloc.GetPageCount(pageSize) >= pageCount);
	}
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::PushBackToObject(const ObjectID& obj, const Atom& data)
{
	PROFILE_FUNCTION();

	// Check if object has any pages
	if (!m_ObjectPages.contains(obj))
	{
		AllocatePages(obj, 1);
	}

	ObjectAllocation& objAlloc = m_ObjectPages[obj];
	const unsigned int pageSize = m_PagedBuffer.GetPageSize();

	// Calculate the current page index for next insertion
	const unsigned int pageIndex = objAlloc.totalElementCount / pageSize;
	const unsigned int pageElemOffset = objAlloc.totalElementCount % pageSize;

	// Allocate new page if needed
	if (pageIndex >= objAlloc.GetPageCount(pageSize))
	{
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] Push back overflow. Allocating page for object {}.",
			m_PagedBuffer.GetName(),
			obj);
		AllocatePages(obj, 1);
	}

	const unsigned int targetPage = objAlloc.endPage;

	m_PagedBuffer[targetPage][pageElemOffset] = data;
	objAlloc.totalElementCount++;
	m_Policy.OnAccess(obj);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::MoveObject(const ObjectID& src, const ObjectID& dst)
{
	PROFILE_FUNCTION();

	if (!m_ObjectPages.contains(src))
	{
		LOG_ERROR(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] Move failed. src object not found (src={})",
			m_PagedBuffer.GetName(),
			src);
		return;
	}

	if (m_ObjectPages.contains(dst))
	{
		LOG_WARN(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] Move overwriting destination object {} (src={}). Existing data will be freed.",
			m_PagedBuffer.GetName(),
			dst,
			src);
		_FreeObject(dst);
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
	PROFILE_FUNCTION();

	if (!m_ObjectPages.contains(obj1) || !m_ObjectPages.contains(obj2))
	{
		LOG_ERROR(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] Swap failed. One or both objects not found (obj1={}, obj2={})",
			m_PagedBuffer.GetName(),
			obj1,
			obj2);
		return;
	}

	std::swap(m_ObjectPages[obj1], m_ObjectPages[obj2]);
	m_Policy.OnAccess(obj1);
	m_Policy.OnAccess(obj2);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::DeallocateObject(const ObjectID& obj)
{
	PROFILE_FUNCTION();

	if (!m_ObjectPages.contains(obj))
		return;

	_FreeObject(obj);
	m_Policy.OnRemove(obj);
	m_ObjectPages.erase(obj);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::ClearObject(const ObjectID& obj)
{
	PROFILE_FUNCTION();

	if (!m_ObjectPages.contains(obj))
		return;

	ObjectAllocation& alloc = m_ObjectPages[obj];

	alloc.totalElementCount = 0;
	m_Policy.OnAccess(obj);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
std::vector<GPUBufferRange> GPUPagedCache<ObjectID, Atom, Policy>::GetObjectBufferRanges(const ObjectID& obj)
{
	PROFILE_FUNCTION();

	assert(m_ObjectPages.contains(obj));
	std::vector<GPUBufferRange> result;

	const ObjectAllocation& alloc = m_ObjectPages.at(obj);
	const unsigned int writePageIdx = (alloc.totalElementCount - 1) / m_PagedBuffer.GetPageSize();
	const unsigned int writePage = alloc.endPage;

	std::unordered_set<uint16_t> pagesSet = _CollectPages(alloc);

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
			size_t usedInLastPage = alloc.totalElementCount % m_PagedBuffer.GetPageSize();
			usedInLastPage = (usedInLastPage == 0 ? m_PagedBuffer.GetPageSize() : usedInLastPage);

			size_t length = pagesRange * m_PagedBuffer.GetPageSize() + usedInLastPage;
			result.emplace_back(startPage * m_PagedBuffer.GetPageSize(), length);

		}
	}

	m_Policy.OnAccess(obj);

	return result;
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::_FreeObject(const ObjectID& obj)
{
	assert(m_ObjectPages.contains(obj));

	ObjectAllocation& alloc = m_ObjectPages[obj];

	if (alloc.startPage == PageNode::NULL_PAGE)
		return;

	uint16_t current = alloc.startPage;

	while (current != PageNode::NULL_PAGE)
	{
		uint16_t next = m_PageNodes[current].next;

		m_PagedBuffer.FreePage(current);

		current = next;
	}

	// unnessary to clear object alloc data, but done for safety
	alloc.startPage = PageNode::NULL_PAGE;
	alloc.endPage = PageNode::NULL_PAGE;
	alloc.totalElementCount = 0;
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::_FreeChain(uint16_t startPage)
{
	if (startPage == PageNode::NULL_PAGE)
		return;

	uint16_t current = startPage;

	while (current != PageNode::NULL_PAGE)
	{
		uint16_t next = m_PageNodes[current].next;

		m_PagedBuffer.FreePage(current);

		current = next;
	}
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::_BuildPageChain(ObjectAllocation& alloc, const std::vector<uint16_t>& pages)
{
	alloc.startPage = pages.front();
	alloc.endPage = pages.back();

	for (size_t i = 0; i < pages.size() - 1; ++i)
	{
		m_PageNodes[pages[i]].next = pages[i + 1];
	}

	m_PageNodes[pages.back()].next = PageNode::NULL_PAGE;
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::_AppendPages(ObjectAllocation& alloc, const std::vector<uint16_t>& pages)
{
	m_PageNodes[alloc.endPage].next = pages.front();

	for (size_t i = 0; i < pages.size() - 1; ++i)
	{
		m_PageNodes[pages[i]].next = pages[i + 1];
	}

	m_PageNodes[pages.back()].next = PageNode::NULL_PAGE;
	alloc.endPage = pages.back();
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
std::unordered_set<uint16_t> GPUPagedCache<ObjectID, Atom, Policy>::_CollectPages(const ObjectAllocation& alloc)
{
	std::unordered_set<uint16_t> objPages;

	if (alloc.startPage == PageNode::NULL_PAGE)
		return objPages;

	uint16_t current = alloc.startPage;

	while (current != PageNode::NULL_PAGE)
	{
		uint16_t next = m_PageNodes[current].next;

		objPages.insert(current);

		current = next;
	}

	return objPages;
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
bool GPUPagedCache<ObjectID, Atom, Policy>::_TryReservePages(const ObjectID& obj, uint16_t pageCount)
{
	std::vector<uint16_t> allocatedPages;
	allocatedPages.reserve(pageCount);

	bool pagesReservedStatus = m_PagedBuffer.ReserveFirstAvaliblePages(pageCount, allocatedPages);
	auto targetAlloc = m_ObjectPages.find(obj);

	if (targetAlloc != m_ObjectPages.end())
	{
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] allocating {} pages for existing object {}",
			m_PagedBuffer.GetName(), pageCount, obj);

		_AppendPages(targetAlloc->second, allocatedPages);
		m_Policy.OnAccess(obj);
	}
	else
	{
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] allocating {} pages for new object {}",
			m_PagedBuffer.GetName(), pageCount, obj);

		ObjectAllocation alloc;
		_BuildPageChain(alloc, allocatedPages);

		m_ObjectPages.emplace(obj, alloc);
		m_Policy.OnInsert(obj);
	}

	return pagesReservedStatus;
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::_EvictAndTakePages(ObjectAllocation& targetAlloc, uint16_t requiredPages)
{
	ObjectID victimID = m_Policy.SelectVictim();
	const unsigned int pageSize = m_PagedBuffer.GetPageSize();

	auto victimIt = m_ObjectPages.find(victimID);
	assert(victimIt != m_ObjectPages.end());

	ObjectAllocation& victimAlloc = victimIt->second;

	size_t pagesNeeded = requiredPages - targetAlloc.GetPageCount(pageSize);
	size_t pagesToTake = std::min(
		static_cast<size_t>(victimAlloc.GetPageCount(pageSize)),
		pagesNeeded
	);

	LOG_DEBUG(EngineSystem::GPU_BUFFER,
		"[GPUPagedLRUCache|{}] Evicting object {} (take={}, free={})",
		m_PagedBuffer.GetName(),
		victimID,
		pagesToTake,
		victimAlloc.GetPageCount(pageSize) - pagesToTake);

	auto splitChain = _SplitVictimChain(victimAlloc, pagesToTake);

	_AttachPages(targetAlloc, splitChain.takeStart, splitChain.takeEnd);

	_FreeChain(splitChain.remainingStart);
	m_ObjectPages.erase(victimID);
	m_Policy.OnRemove(victimID);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
GPUPagedCache<ObjectID, Atom, Policy>::SplitChain GPUPagedCache<ObjectID, Atom, Policy>::_SplitVictimChain(ObjectAllocation& victim, uint16_t pagesToTake)
{
	uint16_t takeStart = victim.startPage;
	uint16_t current = takeStart;
	uint16_t prev = PageNode::NULL_PAGE;

	for (size_t i = 0; i < pagesToTake; ++i)
	{
		prev = current;
		current = m_PageNodes[current].next;
	}

	uint16_t remainingStart = current;

	if (prev != PageNode::NULL_PAGE)
		m_PageNodes[prev].next = PageNode::NULL_PAGE;

	return { 
		.takeStart = takeStart, 
		.takeEnd = prev, 
		.remainingStart = remainingStart 
	};
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::_AttachPages(ObjectAllocation& target, uint16_t start, uint16_t end)
{
	if (target.IsEmpty())
	{
		target.startPage = start;
		target.endPage = end;
	}
	else
	{
		m_PageNodes[target.endPage].next = start;
		target.endPage = end;
	}
}


#endif
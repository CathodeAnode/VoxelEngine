#ifndef GPU_CACHE_ALLOCATOR_TPP
#define GPU_CACHE_ALLOCATOR_TPP

#include "gpu_cache_allocator.h"

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
GPUPagedCache<TObjectID, TAtom, Policy>::GPUPagedCache(bool cpuUpdates)
	: m_PagedBuffer(cpuUpdates)
	, m_PageNodes(cpuUpdates)
	, m_Policy(10000)
{
	PROFILE_FUNCTION();
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
GPUPagedCache<TObjectID, TAtom, Policy>::~GPUPagedCache()
{
	Destroy();
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
bool GPUPagedCache<TObjectID, TAtom, Policy>::Create(GLenum target, size_t pageSize, uint32_t pageCount) noexcept
{
	PROFILE_FUNCTION();

	if (pageCount > PageNode::NULL_PAGE - 1 || pageSize <= 0 || pageCount <= 0) 
		return false;

	constexpr float MAX_LOAD = 0.5f;

	bool result = m_PagedBuffer.Create(target, pageSize, pageCount);
	result &= m_PageNodes.Create(GL_SHADER_STORAGE_BUFFER, pageCount, BufferAccess::ReadWrite); // GL_UNIFORM_BUFFER
	result &= m_ObjectPages.Create(pageCount / MAX_LOAD);
	result &= m_Policy.Create();

	PageNode initValue{ .next = PageNode::NULL_PAGE };
	std::fill_n(m_PageNodes.GetContents(), pageCount, initValue);

	//TODO: add other gpu buffers to calculation
	//TODO: change GPUPagedCache to GPUPaged{type}Cache
	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUPagedCache|{}] reserved {:.2f} Mb of VRAM",
		m_PagedBuffer.GetName(),
		((sizeof(TAtom) * pageCount * pageSize) / 1000000.0f));

	return result;
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
void GPUPagedCache<TObjectID, TAtom, Policy>::Destroy() noexcept
{
	PROFILE_FUNCTION();

	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUPagedCache|{}] Destroyed",
		m_PagedBuffer.GetName());

	m_ObjectPages.Destroy();
	m_PagedBuffer.Destroy();
	m_PageNodes.Destroy();
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
GPUPagedCache<TObjectID, TAtom, Policy>::ObjectAllocation GPUPagedCache<TObjectID, TAtom, Policy>::AllocatePages(const TObjectID& obj, uint32_t pageCount)
{
	PROFILE_FUNCTION();
	
	assert(pageCount > 0);

	ObjectAllocation targetAlloc;
	bool isCached = m_ObjectPages.Find(obj, targetAlloc);

	const unsigned int pageSize = m_PagedBuffer.GetPageSize();

	uint32_t allocatedPages = _TryReservePages(obj, pageCount, targetAlloc);

	while (allocatedPages < pageCount)
	{
		allocatedPages += _EvictAndTakePages(obj, targetAlloc, pageCount - allocatedPages);
	}

	if (isCached)
		m_Policy.OnAccess(targetAlloc.policyHandle);
	else
		targetAlloc.policyHandle = m_Policy.OnInsert(obj);

	m_ObjectPages.Insert(obj, targetAlloc);

	return targetAlloc;
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
void GPUPagedCache<TObjectID, TAtom, Policy>::AllocateObject(const TObjectID& obj, const TAtom* data, size_t count)
{
	DeallocateObject(obj);

	if (count == 0)
	{
		AllocatePages(obj, 1);
		return;
	}

	const size_t pageSize = m_PagedBuffer.GetPageSize();
	const unsigned int pagesNeeded = (count + pageSize - 1) / pageSize;

	ObjectAllocation alloc = AllocatePages(obj, pagesNeeded);

	uint32_t current = alloc.startPage;
	size_t remaining = count;
	size_t index = 0;
	while (current != PageNode::NULL_PAGE && remaining > 0)
	{
		uint32_t next = m_PageNodes[current].next;

		size_t copySize = std::min(remaining, pageSize);
		memcpy(&m_PagedBuffer[current][0], &data[index], copySize * sizeof(TAtom));
		index += copySize;
		remaining -= copySize;

		current = next;
	}

	alloc.totalElementCount = count;
	m_ObjectPages.Insert(obj, alloc);
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
void GPUPagedCache<TObjectID, TAtom, Policy>::PushBackToObject(const TObjectID& obj, const TAtom& data)
{
	PROFILE_FUNCTION();

	ObjectAllocation objAlloc;
	if (!m_ObjectPages.Find(obj, objAlloc))
	{
		objAlloc = AllocatePages(obj, 1);
	}

	const unsigned int pageSize = m_PagedBuffer.GetPageSize();

	// Calculate the current page index for next insertion
	const unsigned int pageIndex = objAlloc.totalElementCount / pageSize;
	const unsigned int pageElemOffset = objAlloc.totalElementCount % pageSize;

	// Allocate new page if needed
	if (pageIndex >= _CountAllocatedPages(objAlloc))
	{
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] Push back overflow. Allocating page for object {}.",
			m_PagedBuffer.GetName(),
			obj);
		objAlloc = AllocatePages(obj, 1);
	}

	const unsigned int targetPage = objAlloc.endPage;

	m_PagedBuffer[targetPage][pageElemOffset] = data;
	objAlloc.totalElementCount++;
	m_Policy.OnAccess(objAlloc.policyHandle);
	m_ObjectPages.Insert(obj, objAlloc);
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
void GPUPagedCache<TObjectID, TAtom, Policy>::MoveObject(const TObjectID& src, const TObjectID& dst)
{
	PROFILE_FUNCTION();

	ObjectAllocation srcObj, dstObj;
	if (!m_ObjectPages.Find(src, srcObj))
	{
		LOG_ERROR(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] Move failed. src object not found (src={})",
			m_PagedBuffer.GetName(),
			src);
		return;
	}

	if (m_ObjectPages.Find(dst, dstObj))
	{
		LOG_WARN(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] Move overwriting destination object {} (src={}). Existing data will be freed.",
			m_PagedBuffer.GetName(),
			dst,
			src);
		_FreeChain(dstObj.startPage);
	}

	m_ObjectPages.Insert(dst, srcObj);
	m_ObjectPages.Erase(src);
	m_Policy.OnRemove(srcObj.policyHandle);
	m_Policy.OnAccess(dstObj.policyHandle);
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
void GPUPagedCache<TObjectID, TAtom, Policy>::Swap(const TObjectID& obj1, const TObjectID& obj2)
{
	// NOTE: This swap is not TAtomic.
	// There is a brief window where one entry is updated before the other,
	// so concurrent readers may observe a temporary inconsistent state

	PROFILE_FUNCTION();

	ObjectAllocation tmp1, tmp2;
	if (!m_ObjectPages.Find(obj1, tmp1) || !m_ObjectPages.Find(obj2, tmp2))
	{
		LOG_ERROR(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] Swap failed. One or both objects not found (obj1={}, obj2={})",
			m_PagedBuffer.GetName(),
			obj1,
			obj2);
		return;
	}

	m_ObjectPages.Insert(obj1, tmp2);
	m_ObjectPages.Insert(obj2, tmp1);
	m_Policy.OnAccess(tmp1.policyHandle);
	m_Policy.OnAccess(tmp2.policyHandle);
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
void GPUPagedCache<TObjectID, TAtom, Policy>::DeallocateObject(const TObjectID& obj)
{
	PROFILE_FUNCTION();

	ObjectAllocation alloc;
	if (!m_ObjectPages.Find(obj, alloc))
		return;

	_FreeChain(alloc.startPage);
	m_Policy.OnRemove(alloc.policyHandle);
	m_ObjectPages.Erase(obj);
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
void GPUPagedCache<TObjectID, TAtom, Policy>::ClearObject(const TObjectID& obj)
{
	PROFILE_FUNCTION();

	ObjectAllocation alloc;
	if (!m_ObjectPages.Find(obj, alloc))
		return;


	alloc.totalElementCount = 0;
	m_Policy.OnAccess(alloc.policyHandle);
	m_ObjectPages.Insert(obj, alloc);
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
std::vector<GPUBufferRange> GPUPagedCache<TObjectID, TAtom, Policy>::GetObjectBufferRanges(const TObjectID& obj)
{
	PROFILE_FUNCTION();

	assert(m_ObjectPages.Contains(obj));
	std::vector<GPUBufferRange> result;

	ObjectAllocation alloc;
	m_ObjectPages.Find(obj, alloc);
	const unsigned int writePageIdx = (alloc.totalElementCount - 1) / m_PagedBuffer.GetPageSize();
	const unsigned int writePage = alloc.endPage;

	std::unordered_set<uint32_t> pagesSet = _CollectPages(alloc);

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

	m_Policy.OnAccess(alloc.policyHandle);

	return result;
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
void GPUPagedCache<TObjectID, TAtom, Policy>::BindCacheData()
{
	m_PagedBuffer.BindBuffer();
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
void GPUPagedCache<TObjectID, TAtom, Policy>::BindCacheLookup(GLint hashMapLocation, GLint nodesLocation, GLint policyLocation)
{
	m_ObjectPages.BindBuffer(hashMapLocation);
	m_PageNodes.BindBufferBase(nodesLocation);
	m_Policy.BindBuffers(policyLocation);
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
void GPUPagedCache<TObjectID, TAtom, Policy>::_FreeChain(uint32_t startPage)
{
	if (startPage == PageNode::NULL_PAGE)
		return;

	uint32_t current = startPage;

	while (current != PageNode::NULL_PAGE)
	{
		uint32_t next = m_PageNodes[current].next;

		m_PageNodes[current].next = PageNode::NULL_PAGE;

		m_PagedBuffer.FreePage(current);

		current = next;
	}
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
void GPUPagedCache<TObjectID, TAtom, Policy>::_BuildPageChain(ObjectAllocation& alloc, const std::vector<uint32_t>& pages)
{
	PROFILE_FUNCTION();

	alloc.startPage = pages.front();
	alloc.endPage = pages.back();

	for (size_t i = 0; i < pages.size() - 1; ++i)
	{
		m_PageNodes[pages[i]].next = pages[i + 1];
	}

	m_PageNodes[pages.back()].next = PageNode::NULL_PAGE;
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
void GPUPagedCache<TObjectID, TAtom, Policy>::_AppendPages(ObjectAllocation& alloc, const std::vector<uint32_t>& pages)
{
	PROFILE_FUNCTION();

	m_PageNodes[alloc.endPage].next = pages.front();

	for (size_t i = 0; i < pages.size() - 1; ++i)
	{
		m_PageNodes[pages[i]].next = pages[i + 1];
	}

	m_PageNodes[pages.back()].next = PageNode::NULL_PAGE;
	alloc.endPage = pages.back();
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
std::unordered_set<uint32_t> GPUPagedCache<TObjectID, TAtom, Policy>::_CollectPages(const ObjectAllocation& alloc) const
{
	PROFILE_FUNCTION();

	std::unordered_set<uint32_t> objPages;

	if (alloc.startPage == PageNode::NULL_PAGE)
		return objPages;

	uint32_t current = alloc.startPage;

	while (current != PageNode::NULL_PAGE)
	{
		uint32_t next = m_PageNodes[current].next;

		objPages.insert(current);

		current = next;
	}

	return objPages;
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
uint32_t GPUPagedCache<TObjectID, TAtom, Policy>::_TryReservePages(const TObjectID& obj, uint32_t pageCount, ObjectAllocation& outAlloc)
{
	PROFILE_FUNCTION();

	std::vector<uint32_t> allocatedPages;
	allocatedPages.reserve(pageCount);

	if (!m_PagedBuffer.ReserveFirstAvaliblePages(pageCount, allocatedPages)) return 0;

	if (outAlloc.startPage != PageNode::NULL_PAGE)
	{
		LOG_TRACE(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] allocating {} pages for existing object {}",
			m_PagedBuffer.GetName(), pageCount, obj);

		_AppendPages(outAlloc, allocatedPages);
	}
	else
	{
		LOG_TRACE(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] allocating {} pages for new object {}",
			m_PagedBuffer.GetName(), pageCount, obj);

		_BuildPageChain(outAlloc, allocatedPages);
	}
	
	return allocatedPages.size();
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
uint32_t GPUPagedCache<TObjectID, TAtom, Policy>::_EvictAndTakePages(const TObjectID& obj, ObjectAllocation& targetAlloc, uint32_t pagesNeeded)
{
	PROFILE_FUNCTION();

	TObjectID victimID = m_Policy.SelectVictim();
	if (victimID == obj)
		return 0;

	const unsigned int pageSize = m_PagedBuffer.GetPageSize();

	ObjectAllocation victimAlloc;
	if (!m_ObjectPages.Find(victimID, victimAlloc))
		return 0;

	uint32_t victimPages = _CountAllocatedPages(victimAlloc);
	uint32_t numPagesToTake = std::min(
		victimPages,
		pagesNeeded
	);

	LOG_DEBUG(EngineSystem::GPU_BUFFER,
		"[GPUPagedLRUCache|{}] Evicting victim={} for requester={} "
		"(victimPages={}, reclaiming={}, freedPages={})",
		m_PagedBuffer.GetName(),
		victimID,
		obj,
		victimPages,
		numPagesToTake,
		victimPages - numPagesToTake);

	auto splitChain = _SplitVictimChain(victimAlloc, numPagesToTake);

	_AttachPages(obj, targetAlloc, splitChain.takeStart, splitChain.takeEnd);

	_FreeChain(splitChain.remainingStart);
	m_Policy.OnRemove(victimAlloc.policyHandle);
	m_ObjectPages.Erase(victimID);

	return numPagesToTake;
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
GPUPagedCache<TObjectID, TAtom, Policy>::SplitChain GPUPagedCache<TObjectID, TAtom, Policy>::_SplitVictimChain(const ObjectAllocation& victim, uint32_t pagesToTake)
{
	PROFILE_FUNCTION();

	assert(pagesToTake > 0);

	uint32_t takeStart = victim.startPage;
	uint32_t current = takeStart;
	uint32_t prev = PageNode::NULL_PAGE;

	for (size_t i = 0; i < pagesToTake; ++i)
	{
		prev = current;
		current = m_PageNodes[current].next;
	}

	uint32_t remainingStart = current;

	if (prev != PageNode::NULL_PAGE)
		m_PageNodes[prev].next = PageNode::NULL_PAGE;

	return { 
		.takeStart = takeStart, 
		.takeEnd = prev, 
		.remainingStart = remainingStart 
	};
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
void GPUPagedCache<TObjectID, TAtom, Policy>::_AttachPages(const TObjectID& obj, ObjectAllocation& target, uint32_t start, uint32_t end)
{
	PROFILE_FUNCTION();

	assert(end != PageNode::NULL_PAGE);

	if (target.IsEmpty())
	{
		target.startPage = start;
	}
	else
	{
		m_PageNodes[target.endPage].next = start;
	}
	target.endPage = end;
	m_PageNodes[end].next = PageNode::NULL_PAGE;
}

template<typename TObjectID, typename TAtom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<TObjectID>, TObjectID>
uint32_t GPUPagedCache<TObjectID, TAtom, Policy>::_CountAllocatedPages(const ObjectAllocation& alloc) const
{
	if (alloc.startPage == PageNode::NULL_PAGE)
		return 0;

	uint32_t count = 0;
	uint32_t current = alloc.startPage;

	while (current != PageNode::NULL_PAGE)
	{
		++count;

		const uint32_t next = m_PageNodes[current].next;

		current = next;
	}

	return count;
}

#endif
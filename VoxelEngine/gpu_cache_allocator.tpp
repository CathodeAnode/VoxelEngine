#ifndef GPU_CACHE_ALLOCATOR_TPP
#define GPU_CACHE_ALLOCATOR_TPP

#include "gpu_cache_allocator.h"

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
GPUPagedCache<ObjectID, Atom, Policy>::GPUPagedCache(bool cpuUpdates)
	: m_PagedBuffer(cpuUpdates)
	, m_PageNodes(cpuUpdates)
	, m_Policy(10000)
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
bool GPUPagedCache<ObjectID, Atom, Policy>::Create(GLenum target, size_t pageSize, uint32_t pageCount) noexcept
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

	constexpr float MAX_LOAD = 0.5f;

	bool result = m_PagedBuffer.Create(target, pageSize, pageCount);
	result |= m_PageNodes.Create(GL_SHADER_STORAGE_BUFFER, pageCount, BufferAccess::ReadWrite); // GL_UNIFORM_BUFFER
	result |= m_ObjectPages.Create(pageCount / MAX_LOAD);
	result |= m_Policy.Create();

	PageNode initValue{ .next = PageNode::NULL_PAGE };
	std::fill_n(m_PageNodes.GetContents(), pageCount, initValue);

	//TODO: add other gpu buffers to calculation
	//TODO: change GPUPagedCache to GPUPaged{type}Cache
	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUPagedCache|{}] reserved {:.2f} Mb of VRAM",
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

	m_ObjectPages.Destroy();
	m_PagedBuffer.Destroy();
	m_PageNodes.Destroy();
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::AllocatePages(const ObjectID& obj, uint32_t pageCount)
{
	PROFILE_FUNCTION();

	const unsigned int pageSize = m_PagedBuffer.GetPageSize();

	bool fullyReserved = _TryReservePages(obj, pageCount);

	ObjectAllocation targetAlloc;
	m_ObjectPages.Find(obj, targetAlloc);
	while (!fullyReserved)
	{
		fullyReserved = _EvictAndTakePages(obj, targetAlloc, pageCount);
	}
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::AllocateObject(const ObjectID& obj, const Atom* data, size_t count)
{
	if (count == 0)
	{
		AllocatePages(obj, 1);
		return;
	}

	const size_t pageSize = m_PagedBuffer.GetPageSize();
	const unsigned int pagesNeeded = (count + pageSize - 1) / pageSize;

	AllocatePages(obj, pagesNeeded);
	ObjectAllocation alloc;
	m_ObjectPages.Find(obj, alloc);

	uint32_t current = alloc.startPage;
	size_t remaining = count;
	size_t index = 0;
	while (current != PageNode::NULL_PAGE && remaining > 0)
	{
		uint32_t next = m_PageNodes[current].next;

		size_t copySize = std::min(remaining, pageSize);
		memcpy(&m_PagedBuffer[current][0], &data[index], copySize * sizeof(Atom)); // copysize needs to be in bytes?
		index += copySize;
		remaining -= copySize;

		current = next;
	}

	alloc.totalElementCount = count;
	m_Policy.OnAccess(alloc.policyHandle);
	m_ObjectPages.Insert(obj, alloc);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::PushBackToObject(const ObjectID& obj, const Atom& data)
{
	PROFILE_FUNCTION();

	// Check if object has any pages
	if (!m_ObjectPages.Contains(obj))
	{
		AllocatePages(obj, 1);
	}

	ObjectAllocation objAlloc;
	m_ObjectPages.Find(obj, objAlloc);
	const unsigned int pageSize = m_PagedBuffer.GetPageSize();

	// Calculate the current page index for next insertion
	const unsigned int pageIndex = objAlloc.totalElementCount / pageSize;
	const unsigned int pageElemOffset = objAlloc.totalElementCount % pageSize;

	// Allocate new page if needed
	if (pageIndex > objAlloc.GetPageCount(pageSize))
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
	m_Policy.OnAccess(objAlloc.policyHandle);
	m_ObjectPages.Insert(obj, objAlloc);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::MoveObject(const ObjectID& src, const ObjectID& dst)
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
		_FreeObject(dst);
	}

	m_ObjectPages.Insert(dst, srcObj);
	m_ObjectPages.Erase(src);
	m_Policy.OnRemove(srcObj.policyHandle);
	m_Policy.OnAccess(dstObj.policyHandle);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::Swap(const ObjectID& obj1, const ObjectID& obj2)
{
	PROFILE_FUNCTION();

	if (!m_ObjectPages.Contains(obj1) || !m_ObjectPages.Contains(obj2))
	{
		LOG_ERROR(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] Swap failed. One or both objects not found (obj1={}, obj2={})",
			m_PagedBuffer.GetName(),
			obj1,
			obj2);
		return;
	}

	// NOTE: This swap is not atomic.
	// There is a brief window where one entry is updated before the other,
	// so concurrent readers may observe a temporary inconsistent state
	ObjectAllocation tmp1, tmp2;
	m_ObjectPages.Find(obj1, tmp1);
	m_ObjectPages.Find(obj2, tmp2);

	m_ObjectPages.Insert(obj1, tmp2);
	m_ObjectPages.Insert(obj2, tmp1);
	m_Policy.OnAccess(tmp1.policyHandle);
	m_Policy.OnAccess(tmp2.policyHandle);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::DeallocateObject(const ObjectID& obj)
{
	PROFILE_FUNCTION();

	ObjectAllocation alloc;
	if (!m_ObjectPages.Find(obj, alloc))
		return;

	_FreeObject(obj);
	m_Policy.OnRemove(alloc.policyHandle);
	m_ObjectPages.Erase(obj);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::ClearObject(const ObjectID& obj)
{
	PROFILE_FUNCTION();

	ObjectAllocation alloc;
	if (!m_ObjectPages.Find(obj, alloc))
		return;


	alloc.totalElementCount = 0;
	m_Policy.OnAccess(alloc.policyHandle);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
std::vector<GPUBufferRange> GPUPagedCache<ObjectID, Atom, Policy>::GetObjectBufferRanges(const ObjectID& obj)
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

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::BindCacheData()
{
	m_PagedBuffer.BindBuffer();
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::BindCacheLookup(GLint hashMapLocation, GLint nodesLocation, GLint policyLocation)
{
	m_ObjectPages.BindBuffer(hashMapLocation);
	m_PageNodes.BindBufferBase(nodesLocation);
	m_Policy.BindBuffers(policyLocation);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::_FreeObject(const ObjectID& obj)
{
	assert(m_ObjectPages.Contains(obj));

	ObjectAllocation alloc;
	m_ObjectPages.Find(obj, alloc);

	if (alloc.startPage == PageNode::NULL_PAGE)
		return;

	uint32_t current = alloc.startPage;

	while (current != PageNode::NULL_PAGE)
	{
		uint32_t next = m_PageNodes[current].next;

		m_PagedBuffer.FreePage(current);

		current = next;
	}

	// unnessary to clear object alloc data, but done for safety
	alloc.startPage = PageNode::NULL_PAGE;
	alloc.endPage = PageNode::NULL_PAGE;
	alloc.totalElementCount = 0;
	m_ObjectPages.Insert(obj, alloc);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::_FreeChain(uint32_t startPage)
{
	if (startPage == PageNode::NULL_PAGE)
		return;

	uint32_t current = startPage;

	while (current != PageNode::NULL_PAGE)
	{
		uint32_t next = m_PageNodes[current].next;

		m_PagedBuffer.FreePage(current);

		current = next;
	}
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::_BuildPageChain(ObjectAllocation& alloc, const std::vector<uint32_t>& pages)
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

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::_AppendPages(ObjectAllocation& alloc, const std::vector<uint32_t>& pages)
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

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
std::unordered_set<uint32_t> GPUPagedCache<ObjectID, Atom, Policy>::_CollectPages(const ObjectAllocation& alloc)
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

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
bool GPUPagedCache<ObjectID, Atom, Policy>::_TryReservePages(const ObjectID& obj, uint32_t pageCount)
{
	PROFILE_FUNCTION();

	std::vector<uint32_t> allocatedPages;
	allocatedPages.reserve(pageCount);

	bool pagesReservedStatus = m_PagedBuffer.ReserveFirstAvaliblePages(pageCount, allocatedPages);
	ObjectAllocation targetAlloc;

	if (m_ObjectPages.Find(obj, targetAlloc))
	{
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] allocating {} pages for existing object {}",
			m_PagedBuffer.GetName(), pageCount, obj);

		_AppendPages(targetAlloc, allocatedPages);
		m_Policy.OnAccess(targetAlloc.policyHandle);
	}
	else
	{
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUPagedCache|{}] allocating {} pages for new object {}",
			m_PagedBuffer.GetName(), pageCount, obj);

		ObjectAllocation alloc;
		_BuildPageChain(alloc, allocatedPages);

		alloc.policyHandle = m_Policy.OnInsert(obj);
		m_ObjectPages.Insert(obj, alloc);
	}

	return pagesReservedStatus;
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
bool GPUPagedCache<ObjectID, Atom, Policy>::_EvictAndTakePages(const ObjectID& obj, ObjectAllocation& targetAlloc, uint32_t requiredPages)
{
	PROFILE_FUNCTION();

	ObjectID victimID = m_Policy.SelectVictim();
	const unsigned int pageSize = m_PagedBuffer.GetPageSize();

	ObjectAllocation victimAlloc;
	m_ObjectPages.Find(victimID, victimAlloc);
	assert(m_ObjectPages.Contains(victimID));

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

	_AttachPages(obj, targetAlloc, splitChain.takeStart, splitChain.takeEnd);

	_FreeChain(splitChain.remainingStart);
	m_Policy.OnRemove(victimAlloc.policyHandle);
	m_ObjectPages.Erase(victimID);

	return pagesNeeded > victimAlloc.GetPageCount(pageSize);
}

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
GPUPagedCache<ObjectID, Atom, Policy>::SplitChain GPUPagedCache<ObjectID, Atom, Policy>::_SplitVictimChain(ObjectAllocation& victim, uint32_t pagesToTake)
{
	PROFILE_FUNCTION();

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

template<typename ObjectID, typename Atom, template<typename> typename Policy>
	requires EvictionPolicy<Policy<ObjectID>, ObjectID>
void GPUPagedCache<ObjectID, Atom, Policy>::_AttachPages(const ObjectID& obj, ObjectAllocation& target, uint32_t start, uint32_t end)
{
	PROFILE_FUNCTION();

	if (target.IsEmpty())
	{
		target.startPage = start;
	}
	else
	{
		m_PageNodes[target.endPage].next = start;
	}
	target.endPage = end;
	m_ObjectPages.Insert(obj, target);
}

#endif
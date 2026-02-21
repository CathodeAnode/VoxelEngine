#include "gpu_buffer_allocator.h"

// LOGGING Format: [type|id] event (eventinfo1=, eventinfo2=, ...)
// types: Persist, Ring, Orphan, Cache

// someway to convay that other buffer allocators are just wrappers for GPUPersistentlyMappedBuffer

// log levels:
// trace for per frame calls
// debug for internal logic such as evictions, wrap around in ring buffer
// info for creation & destruction

template<typename Atom, IBufferLockManager LockManager>
GPUPersistentlyMappedBuffer<Atom, LockManager>::GPUPersistentlyMappedBuffer(bool _cpuUpdates)
	: m_LockManager(_cpuUpdates)
	, m_BufferContents()
	, m_Name()
	, m_Target()
{
	PROFILE_FUNCTION();
}

template<typename Atom, IBufferLockManager LockManager>
GPUPersistentlyMappedBuffer<Atom, LockManager>::~GPUPersistentlyMappedBuffer()
{
	LOG_INFO(EngineSystem::GPU_BUFFER, "[GPUPersistentlyMappedBuffer|ID={}] Destroyed", m_Name);
	Destroy();
}

template<typename Atom, IBufferLockManager LockManager>
bool GPUPersistentlyMappedBuffer<Atom, LockManager>::Create(GLenum _target, GLuint _count, BufferAccess access)
{
	PROFILE_FUNCTION();

	if (m_BufferContents) {
		LOG_WARN(EngineSystem::GPU_BUFFER,
			"[GPUPersistentlyMappedBuffer|{}] Create called on already-initialized buffer, destroying old buffer",
			m_Name);
		Destroy();
	}

	m_Target = _target;
	m_CountAtoms = _count;
	// This code currently doesn't care about the alignment of the returned memory. This could potentially
	// cause a crash, but since implementations are likely to return us memory that is at lest aligned
	// on a 64-byte boundary we're okay with this for now. 
	// A robust implementation would ensure that the memory returned had enough slop that it could deal
	// with it's own alignment issues, at least. That's more work than I want to do right this second.
	GLbitfield flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

	switch (access) 
	{
	case BufferAccess::WriteOnly:
		flags |= GL_MAP_WRITE_BIT;
		break;
	case BufferAccess::ReadWrite:
		flags |= GL_MAP_WRITE_BIT | GL_MAP_READ_BIT;
		break;
	case BufferAccess::ReadOnly:
		flags |= GL_MAP_READ_BIT;
		break;
	}

	glGenBuffers(1, &m_Name);
	glBindBuffer(m_Target, m_Name);
	glBufferStorage(m_Target, sizeof(Atom) * _count, nullptr, flags | GL_DYNAMIC_STORAGE_BIT);
	m_BufferContents = reinterpret_cast<Atom*>(glMapBufferRange(m_Target, 0, sizeof(Atom) * _count, flags));

	if (!m_BufferContents) {
		LOG_ERROR(EngineSystem::GPU_BUFFER,
			"[GPUPersistentlyMappedBuffer|{}] glMapBufferRange failed (target={}, bytes={})",
			m_Name, GPUAllocatorsUtils::ToString(m_Target), sizeof(Atom) * _count);
		return false;
	}

	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUPersistentlyMappedBuffer|{}] Created (target={}, atomCount={}, atomSize={}, access={})",
		m_Name, GPUAllocatorsUtils::ToString(_target), _count, sizeof(Atom), GPUAllocatorsUtils::ToString(access));

	return true;
}

template<typename Atom, IBufferLockManager LockManager>
void GPUPersistentlyMappedBuffer<Atom, LockManager>::Destroy()
{
	PROFILE_FUNCTION();

	if (!m_Name) {
		LOG_WARN(EngineSystem::GPU_BUFFER, "Destroy called on empty GPUPersistentlyMappedBuffer");
		return;
	}

	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUPersistentlyMappedBuffer|{}] Destroyed (target={})",
		m_Name, GPUAllocatorsUtils::ToString(m_Target));

	glBindBuffer(m_Target, m_Name);
	glUnmapBuffer(m_Target);
	glDeleteBuffers(1, &m_Name);

	m_BufferContents = nullptr;
	m_Name = 0;
}

template<typename Atom, IBufferLockManager LockManager>
void GPUPersistentlyMappedBuffer<Atom, LockManager>::WaitForLockedRange(size_t _lockBegin, size_t _lockLength)
{
	LOG_TRACE(EngineSystem::GPU_BUFFER,
		"Waiting for locked range (ID={}, begin={}, length={}, range=[{}, {}))",
		m_Name,
		_lockBegin,
		_lockLength,
		_lockBegin,
		(_lockBegin + _lockLength));

	m_LockManager.WaitForLockedRange(_lockBegin * sizeof(Atom), _lockLength * sizeof(Atom));
}

template<typename Atom, IBufferLockManager LockManager>
void GPUPersistentlyMappedBuffer<Atom, LockManager>::LockRange(size_t _lockBegin, size_t _lockLength)
{
	LOG_TRACE(EngineSystem::GPU_BUFFER,
		"Locking range (ID={}, begin={}, length={}, range=[{}, {}))",
		m_Name,
		_lockBegin,
		_lockLength,
		_lockBegin,
		(_lockBegin + _lockLength));

	m_LockManager.LockRange(_lockBegin * sizeof(Atom), _lockLength * sizeof(Atom));
}

template<typename Atom, IBufferLockManager LockManager>
void GPUPersistentlyMappedBuffer<Atom, LockManager>::BindBuffer()
{
	LOG_TRACE(EngineSystem::GPU_BUFFER, "Binding buffer (ID={}, target={})", m_Name, GPUAllocatorsUtils::ToString(m_Target));
	glBindBuffer(m_Target, m_Name);
}

template<typename Atom, IBufferLockManager LockManager>
void GPUPersistentlyMappedBuffer<Atom, LockManager>::BindBufferBase(GLuint _index)
{
	LOG_TRACE(EngineSystem::GPU_BUFFER,
		"Binding buffer base (ID={}, target={}, index={})",
		m_Name, m_Target, _index);
	glBindBufferBase(m_Target, _index, m_Name);
}

template<typename Atom, IBufferLockManager LockManager>
void GPUPersistentlyMappedBuffer<Atom, LockManager>::BindBufferRange(GLuint _index, size_t _head, size_t _count)
{
	LOG_TRACE(EngineSystem::GPU_BUFFER,
		"Binding buffer range (ID={}, target={}, index={}, head={}, count={}, bytes=[{}, {}))",
		m_Name,
		GPUAllocatorsUtils::ToString(m_Target),
		_index,
		_head,
		_count,
		_head * sizeof(Atom),
		(_head + _count) * sizeof(Atom));

	glBindBufferRange(m_Target, _index, m_Name , _head * sizeof(Atom), _count * sizeof(Atom));
}

// ------------------------------------------------------------------------------------------------------------------

template<typename Atom, IBufferLockManager LockManager>
GPUCircularBuffer<Atom, LockManager>::GPUCircularBuffer(bool _cpuUpdates)
	: m_Buffer(_cpuUpdates)
{
	PROFILE_FUNCTION();
}

template<typename Atom, IBufferLockManager LockManager>
bool GPUCircularBuffer<Atom, LockManager>::Create(GLenum _target, GLuint _count, BufferAccess access)
{
	PROFILE_FUNCTION();
	m_Head = 0;
	bool result = m_Buffer.Create(_target, _count, access);
	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUCircularBuffer|{}] Created (target={}, capacity={})",
		m_Buffer.GetName(), GPUAllocatorsUtils::ToString(_target), _count);
	return result;
}

template<typename Atom, IBufferLockManager LockManager>
void GPUCircularBuffer<Atom, LockManager>::Destroy()
{
	PROFILE_FUNCTION();
	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUCircularBuffer|{}] Destroyed",
		m_Buffer.m_Name());
	m_Buffer.Destroy();
	m_Head = 0;
}

template<typename Atom, IBufferLockManager LockManager>
Atom* GPUCircularBuffer<Atom, LockManager>::Reserve(size_t _count)
{
	if (_count > m_Buffer.GetSize()) {
		LOG_ERROR(EngineSystem::GPU_BUFFER,
			"[GPUCircularBuffer|{}] Reserve request exceeds circular buffer capacity (requested={}, capacity={})",
			m_Buffer.GetName(), _count, m_Buffer.GetSize());
		return nullptr;
	}

	size_t lockStart = m_Head;

	if (lockStart + _count > m_Buffer.GetSize()) {
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUCircularBuffer|{}] buffer wrap: head={}, requested={}, capacity={}",
			m_Buffer.GetName(), lockStart, _count, m_Buffer.GetSize());
		// Need to wrap here.
		lockStart = 0;
		m_Head = 0;
	}

	m_Buffer.WaitForLockedRange(lockStart, _count);
	return &m_Buffer.GetContents()[lockStart];
}

template<typename Atom, IBufferLockManager LockManager>
Atom* GPUCircularBuffer<Atom, LockManager>::ReserveRange(size_t start, size_t count)
{
	if (count > m_Buffer.GetSize()) {
		LOG_ERROR(EngineSystem::GPU_BUFFER,
			"[GPUCircularBuffer|{}] Reserve request exceeds circular buffer capacity (requested={}, capacity={})",
			m_Buffer.GetName(), count, m_Buffer.GetSize());
		return nullptr;
	}


	if (start + count > m_Buffer.GetSize()) {
		LOG_DEBUG(EngineSystem::GPU_BUFFER,
			"[GPUCircularBuffer|{}] buffer wrap: head={}, requested={}, capacity={}",
			m_Buffer.GetName(), start, count, m_Buffer.GetSize());
		// Need to wrap here.
		start = 0;
	}

	m_Buffer.WaitForLockedRange(start, count);
	return &m_Buffer.GetContents()[start];
}

template<typename Atom, IBufferLockManager LockManager>
void GPUCircularBuffer<Atom, LockManager>::OnUsageComplete(size_t _count)
{
	assert(_count > 0);
	assert(_count <= m_Buffer.GetSize());
	m_Buffer.LockRange(m_Head, _count);
	m_Head = (m_Head + _count) % m_Buffer.GetSize();
}

template<typename Atom, IBufferLockManager LockManager>
void GPUCircularBuffer<Atom, LockManager>::BindBuffer()
{
	m_Buffer.BindBuffer();
}

template<typename Atom, IBufferLockManager LockManager>
void GPUCircularBuffer<Atom, LockManager>::BindBufferBase(GLuint _index)
{
	m_Buffer.BindBufferBase(_index);
}


template<typename Atom, IBufferLockManager LockManager>
void GPUCircularBuffer<Atom, LockManager>::BindBufferHeadRange(GLuint _index, size_t _count)
{
	assert(m_Head + _count <= m_Buffer.GetSize());
	m_Buffer.BindBufferRange(_index, m_Head, _count);
}

template<typename Atom, IBufferLockManager LockManager>
void GPUCircularBuffer<Atom, LockManager>::BindBufferRange(GLuint _index, size_t _offset, size_t _count)
{
	m_Buffer.BindBufferRange(_index, _offset, _count);
}

// ------------------------------------------------------------------------------------------------------------------

template<typename Atom, IBufferLockManager LockManager>
inline GPUOrphanBuffer<Atom, LockManager>::GPUOrphanBuffer(bool _cpuUpdates)
	: m_CircularBuffer(_cpuUpdates)
{
	PROFILE_FUNCTION();
}

template<typename Atom, IBufferLockManager LockManager>
bool GPUOrphanBuffer<Atom, LockManager>::Create(GLenum target, GLuint countPerBuffer, uint8_t numOfBuffers, BufferAccess access)
{
	assert(numOfBuffers > 0);
	PROFILE_FUNCTION();


	m_CountPerBuffer = countPerBuffer;
	GLuint totalCount = m_CountPerBuffer * numOfBuffers;
	m_Tail = 0;

	bool result = m_CircularBuffer.Create(target, totalCount, access);

	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUOrphanBuffer|{}] Created (target={}, capacity={}, buffers={})",
		m_CircularBuffer.GetName(), GPUAllocatorsUtils::ToString(target), countPerBuffer, numOfBuffers);

	return result;
}

template<typename Atom, IBufferLockManager LockManager>
void GPUOrphanBuffer<Atom, LockManager>::Destroy()
{
	PROFILE_FUNCTION();

	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUOrphanBuffer|{}] Destroyed",
		m_CircularBuffer.m_Name());

	m_CountPerBuffer = 0;
	m_Tail = 0;
	m_CircularBuffer.Destroy();
}

template<typename Atom, IBufferLockManager LockManager>
void GPUOrphanBuffer<Atom, LockManager>::AdvanceHead()
{
	m_CircularBuffer.OnUsageComplete(m_CountPerBuffer);
}

template<typename Atom, IBufferLockManager LockManager>
void GPUOrphanBuffer<Atom, LockManager>::AdvanceTail()
{
	m_Tail = (m_Tail + m_CountPerBuffer) % m_CircularBuffer.GetSize();
}

template<typename Atom, IBufferLockManager LockManager>
void GPUOrphanBuffer<Atom, LockManager>::BindHeadBuffer(GLuint index)
{
	m_CircularBuffer.BindBufferHeadRange(index, m_CountPerBuffer);
}

template<typename Atom, IBufferLockManager LockManager>
void GPUOrphanBuffer<Atom, LockManager>::BindHeadBufferRange(GLuint index, size_t count)
{
	m_CircularBuffer.BindBufferHeadRange(index, count);
}

template<typename Atom, IBufferLockManager LockManager>
void GPUOrphanBuffer<Atom, LockManager>::BindTailBuffer(GLuint index)
{
	m_CircularBuffer.BindBufferRange(index, m_Tail, m_CountPerBuffer);
}

template<typename Atom, IBufferLockManager LockManager>
void GPUOrphanBuffer<Atom, LockManager>::BindTailBufferRange(GLuint index, size_t count)
{
	m_CircularBuffer.BindBufferRange(index, m_Tail, count);
}

// ------------------------------------------------------------------------------------------------------------------

template<typename Atom>
GPUPagedBuffer<Atom>::GPUPagedBuffer()
	: m_AtomCount(0)
	, m_MaxAtomCount(0)
	, m_Name(0)
{
	m_PageTable.reserve(m_KInitialPageTableCapacity);
}

template<typename Atom>
GPUPagedBuffer<Atom>::~GPUPagedBuffer()
{
	Destroy();
}

template<typename Atom>
bool GPUPagedBuffer<Atom>::Create(GLenum _target, GLuint _count) noexcept
{
	m_Target = _target;
	m_MaxAtomCount = _count;
	
	if (m_Name != 0) return false;

	glGenBuffers(1, &m_Name);
	glBindBuffer(_target, m_Name);
	glBufferData(_target, _count * sizeof(Atom), nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(_target, 0);

	return true;
}

template<typename Atom>
void GPUPagedBuffer<Atom>::Destroy() noexcept
{
	glDeleteBuffers(1, &m_Name);
}

template<typename Atom>
size_t GPUPagedBuffer<Atom>::UploadPageData(const std::vector<Atom>& data) noexcept
{
	const unsigned int count = data.size();
	glBindBuffer(m_Target, m_Name);
	glBufferSubData(m_Target, m_AtomCount * sizeof(Atom), count * sizeof(Atom), data.data());
	glBindBuffer(m_Target, 0);

	m_PageTable.push_back({m_AtomCount, count });
	m_AtomCount += count;
	return m_PageTable.size() - 1;
}

template<typename Atom>
bool GPUPagedBuffer<Atom>::UpdatePage(const size_t& pageId, const std::vector<Atom>& data) noexcept
{
	const size_t newPageCount = data.size();

	//get page offsets
	Page page = GetPageOffset(pageId);
	if (page.IsNull()) {
		return false;
	}

	// shift subsequent pages according to new page update
	const size_t oldNextPageIndex = page.index + page.size;
	const size_t newNextPageIndex = page.index + newPageCount;
	const size_t subsequentPagesAtomCount = (m_AtomCount - page.index) + page.size;
	move(oldNextPageIndex, newNextPageIndex, subsequentPagesAtomCount);

	// update page data on gpu
	glBindBuffer(m_Target, m_Name);
	glBufferSubData(m_Target, page.index * sizeof(Atom), newPageCount * sizeof(Atom), data.data());
	glBindBuffer(m_Target, 0);

	// update m_Buffer state in data structure
	m_PageTable[pageId].size = newPageCount;
	const size_t countDelta = newPageCount - page.size;
	for (int i = pageId + 1; i < m_PageTable.size(); i++)
	{
		m_PageTable[i].index += countDelta;
	}

	m_AtomCount += countDelta;
	
	return true;
}

template<typename Atom>
Page GPUPagedBuffer<Atom>::GetPageOffset(const size_t& pageId) noexcept
{
	// page does not exisit
	if (pageId >= m_PageTable.size()) {
		return Page(0, 0);
	}

	return m_PageTable[pageId];
}

template<typename Atom>
void GPUPagedBuffer<Atom>::move(size_t srcIndex, size_t dstIndex, size_t length)
{
	// if intervials overlap
	if (srcIndex < (dstIndex + length)
		&& dstIndex < (srcIndex + length)) {

		// create temp m_Buffer to copy current data into
		unsigned int copyBuffer;
		glGenBuffers(1, &copyBuffer);
		glBindBuffer(GL_COPY_WRITE_BUFFER, copyBuffer);
		glBufferData(GL_COPY_WRITE_BUFFER, length * sizeof(Atom), nullptr, GL_DYNAMIC_COPY);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

		// copy data into temp copy m_Buffer
		glBindBuffer(GL_COPY_READ_BUFFER, m_Name);
		glBindBuffer(GL_COPY_WRITE_BUFFER, copyBuffer);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcIndex * sizeof(Atom), 0, length * sizeof(Atom));

		// copy from temp m_Buffer to move location
		glBindBuffer(GL_COPY_READ_BUFFER, copyBuffer);
		glBindBuffer(GL_COPY_WRITE_BUFFER, m_Name);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, dstIndex * sizeof(Atom), length * sizeof(Atom));
		glBindBuffer(GL_COPY_READ_BUFFER, 0);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

		// delete temp copy m_Buffer
		glDeleteBuffers(1, &copyBuffer);
	}
	else {
		glBindBuffer(GL_COPY_READ_BUFFER, m_Name);
		glBindBuffer(GL_COPY_WRITE_BUFFER, m_Name);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcIndex * sizeof(Atom), dstIndex * sizeof(Atom), sizeof(Atom) * length);
		glBindBuffer(GL_COPY_READ_BUFFER, 0);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	}
}

// ------------------------------------------------------------------------------------------------------------------
template<typename Atom, typename ObjectID>
GPUPagedLRUCache<Atom, ObjectID>::GPUPagedLRUCache(bool cpuUpdates)
	: m_Buffer(cpuUpdates)
{
	PROFILE_FUNCTION();
}

template<typename Atom, typename ObjectID>
GPUPagedLRUCache<Atom, ObjectID>::~GPUPagedLRUCache()
{
	Destroy();
}

template<typename Atom, typename ObjectID>
bool GPUPagedLRUCache<Atom, ObjectID>::Create(GLenum target, size_t pageSize, size_t pageCount) noexcept
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

template<typename Atom, typename ObjectID>
void GPUPagedLRUCache<Atom, ObjectID>::Destroy() noexcept
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

template<typename Atom, typename ObjectID>
void GPUPagedLRUCache<Atom, ObjectID>::AllocatePages(const ObjectID& obj, unsigned int pages)
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

template<typename Atom, typename ObjectID>
void GPUPagedLRUCache<Atom, ObjectID>::PushBackToObject(const ObjectID& obj, const Atom& data)
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

template<typename Atom, typename ObjectID>
void GPUPagedLRUCache<Atom, ObjectID>::MoveObject(const ObjectID& src, const ObjectID& dst)
{
	_FreePages(m_ObjectMapping[src].pages);
	m_ObjectMapping[dst] = std::move(m_ObjectMapping[src]);
	m_ObjectMapping.erase(src);
	m_ObjectAccessHistory.erase(src);
	_MarkRecentlyUsed(dst);
}

template<typename Atom, typename ObjectID>
void GPUPagedLRUCache<Atom, ObjectID>::Swap(const ObjectID& obj1, const ObjectID& obj2)
{
	std::swap(m_ObjectMapping[obj1], m_ObjectMapping[obj2]);
	_MarkRecentlyUsed(obj1);
	_MarkRecentlyUsed(obj2);
}

template<typename Atom, typename ObjectID>
void GPUPagedLRUCache<Atom, ObjectID>::DeallocateObject(const ObjectID& obj)
{
	if (!m_ObjectMapping.contains(obj))
		return;

	ObjectAllocationData& alloc = m_ObjectMapping[obj];

	_FreePages(alloc.pages);
	m_ObjectAccessHistory.erase(alloc.lruIterator);
	m_ObjectMapping.erase(obj);
}

template<typename Atom, typename ObjectID>
void GPUPagedLRUCache<Atom, ObjectID>::ClearObject(const ObjectID& obj)
{
	if (!m_ObjectMapping.contains(obj))
		return;

	ObjectAllocationData& alloc = m_ObjectMapping[obj];

	alloc.count = 0;
	// LRU policy is not updated for object
}

template<typename Atom, typename ObjectID>
std::vector<GPUBufferRange> GPUPagedLRUCache<Atom, ObjectID>::GetObjectBufferRanges(const ObjectID& obj)
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


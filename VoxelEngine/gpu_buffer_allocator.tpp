#include "gpu_buffer_allocator.h"

// LOGGING Format: [type|id] event (eventinfo1=, eventinfo2=, ...)
// types: Persist, Ring, Orphan, Cache

// someway to convay that other buffer allocators are just wrappers for GPUPersistentlyMappedBuffer

// log levels:
// trace for per frame calls
// debug for internal logic such as evictions, wrap around in ring buffer
// info for creation & destruction

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
GPUPersistentlyMappedBuffer<Atom, LockManager>::GPUPersistentlyMappedBuffer(bool _cpuUpdates)
	: m_LockManager(_cpuUpdates)
	, m_BufferContents()
	, m_Name()
	, m_Target()
{
	PROFILE_FUNCTION();
}

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
GPUPersistentlyMappedBuffer<Atom, LockManager>::~GPUPersistentlyMappedBuffer()
{
	Destroy();
}

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
Atom& GPUPersistentlyMappedBuffer<Atom, LockManager>::operator[](size_t index)
{
	assert(index < m_CountAtoms && "Index out of bounds");
	return m_BufferContents[index];
}

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
const Atom& GPUPersistentlyMappedBuffer<Atom, LockManager>::operator[](size_t index) const
{
	assert(index < m_CountAtoms && "Index out of bounds");
	return m_BufferContents[index];
}

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
bool GPUPersistentlyMappedBuffer<Atom, LockManager>::Create(GLenum _target, GLuint _count, BufferAccess access)
{
	PROFILE_FUNCTION();

	if (m_BufferContents) {
		LOG_WARN(EngineSystem::GPU_BUFFER,
			"[GPUPersistentlyMappedBuffer|{}] Create called on already-initialized buffer, destroying old buffer",
			m_Name);
		return false;
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

	glBindBuffer(m_Target, 0);

	return true;
}

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
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

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
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

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
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

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
void GPUPersistentlyMappedBuffer<Atom, LockManager>::BindBuffer()
{
	LOG_TRACE(EngineSystem::GPU_BUFFER, "Binding buffer (ID={}, target={})", m_Name, GPUAllocatorsUtils::ToString(m_Target));
	glBindBuffer(m_Target, m_Name);
}

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
void GPUPersistentlyMappedBuffer<Atom, LockManager>::BindBufferBase(GLuint _index)
{
	LOG_TRACE(EngineSystem::GPU_BUFFER,
		"Binding buffer base (ID={}, target={}, index={})",
		m_Name, m_Target, _index);
	glBindBufferBase(m_Target, _index, m_Name);
}

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
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

	assert(_head % GPUAllocatorsUtils::GetOffsetAlignment(m_Target) == 0); // TODO: add assertion error message

	glBindBufferRange(m_Target, _index, m_Name , _head * sizeof(Atom), _count * sizeof(Atom));
}

// ------------------------------------------------------------------------------------------------------------------

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
GPUCircularBuffer<Atom, LockManager>::GPUCircularBuffer(bool _cpuUpdates)
	: m_Buffer(_cpuUpdates)
{
	PROFILE_FUNCTION();
}

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
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

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
void GPUCircularBuffer<Atom, LockManager>::Destroy()
{
	PROFILE_FUNCTION();
	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUCircularBuffer|{}] Destroyed",
		m_Buffer.GetName());
	m_Buffer.Destroy();
	m_Head = 0;
}

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
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

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
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

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
void GPUCircularBuffer<Atom, LockManager>::OnUsageComplete(size_t _count)
{
	assert(_count <= m_Buffer.GetSize());
	m_Buffer.LockRange(m_Head, _count);
	m_Head = (m_Head + _count) % m_Buffer.GetSize();
}

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
void GPUCircularBuffer<Atom, LockManager>::BindBuffer()
{
	m_Buffer.BindBuffer();
}

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
void GPUCircularBuffer<Atom, LockManager>::BindBufferBase(GLuint _index)
{
	m_Buffer.BindBufferBase(_index);
}


template<GPUSafeStruct Atom, IBufferLockManager LockManager>
void GPUCircularBuffer<Atom, LockManager>::BindBufferHeadRange(GLuint _index, size_t _count)
{
	assert(m_Head + _count <= m_Buffer.GetSize());
	m_Buffer.BindBufferRange(_index, m_Head, _count);
}

template<GPUSafeStruct Atom, IBufferLockManager LockManager>
void GPUCircularBuffer<Atom, LockManager>::BindBufferRange(GLuint _index, size_t _offset, size_t _count)
{
	m_Buffer.BindBufferRange(_index, _offset, _count);
}

// ------------------------------------------------------------------------------------------------------------------

template<GPUSafeStruct Atom, size_t FRAME_COUNT>
inline GPUOrphanBuffer<Atom, FRAME_COUNT>::GPUOrphanBuffer(bool _cpuUpdates)
	: m_Buffer(_cpuUpdates)
{
	PROFILE_FUNCTION();
}

template<GPUSafeStruct Atom, size_t FRAME_COUNT>
bool GPUOrphanBuffer<Atom, FRAME_COUNT>::Create(GLenum target, GLuint countPerBuffer, BufferAccess access)
{
	assert(FRAME_COUNT > 0);
	PROFILE_FUNCTION();

	GLuint alignment = GPUAllocatorsUtils::GetOffsetAlignment(target);
	GLuint alignedSize = (countPerBuffer + alignment - 1) / alignment * alignment;
	m_FrameSize = alignedSize;
	m_CurrentFrame = 0;

	const GLuint totalCount = m_FrameSize * FRAME_COUNT;

	bool result = m_Buffer.Create(target, totalCount, access);

	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUOrphanBuffer] Created (target={}, frameSize={}, frames={})",
		GPUAllocatorsUtils::ToString(target), countPerBuffer, FRAME_COUNT);

	return result;
}

template<GPUSafeStruct Atom, size_t FRAME_COUNT>
void GPUOrphanBuffer<Atom, FRAME_COUNT>::Destroy()
{
	PROFILE_FUNCTION();

	m_FrameSize = 0;
	m_CurrentFrame = 0;

	m_Buffer.Destroy();
}

template<GPUSafeStruct Atom, size_t FRAME_COUNT>
void GPUOrphanBuffer<Atom, FRAME_COUNT>::Commit()
{
	m_CurrentFrame = (m_CurrentFrame + 1) % FRAME_COUNT;
}

template<GPUSafeStruct Atom, size_t FRAME_COUNT>
void GPUOrphanBuffer<Atom, FRAME_COUNT>::BindPreviousFrame(GLuint index)
{
	const size_t frame = (m_CurrentFrame + FRAME_COUNT - 1) % FRAME_COUNT;
	const size_t offset = _GetOffset(frame);

	m_Buffer.BindBufferRange(index, offset, m_FrameSize);
}

template<GPUSafeStruct Atom, size_t FRAME_COUNT>
void GPUOrphanBuffer<Atom, FRAME_COUNT>::BindCurrentFrame(GLuint index)
{
	const size_t offset = _GetOffset(m_CurrentFrame);

	m_Buffer.BindBufferRange(index, offset, m_FrameSize);
}

template<GPUSafeStruct Atom, size_t FRAME_COUNT>
size_t GPUOrphanBuffer<Atom, FRAME_COUNT>::GetPrevFrameOffset() const
{
	const size_t frame = (m_CurrentFrame + FRAME_COUNT - 1) % FRAME_COUNT;
	const size_t offset = _GetOffset(frame);

	return offset;
}


// ------------------------------------------------------------------------------------------------------------------

//template<GPUSafeStruct Atom>
//GPUPagedBuffer<Atom>::GPUPagedBuffer()
//	: m_AtomCount(0)
//	, m_MaxAtomCount(0)
//	, m_Name(0)
//{
//	m_PageTable.reserve(m_KInitialPageTableCapacity);
//}
//
//template<GPUSafeStruct Atom>
//GPUPagedBuffer<Atom>::~GPUPagedBuffer()
//{
//	Destroy();
//}
//
//template<GPUSafeStruct Atom>
//bool GPUPagedBuffer<Atom>::Create(GLenum _target, GLuint _count) noexcept
//{
//	m_Target = _target;
//	m_MaxAtomCount = _count;
//	
//	if (m_Name != 0) return false;
//
//	glGenBuffers(1, &m_Name);
//	glBindBuffer(_target, m_Name);
//	glBufferData(_target, _count * sizeof(Atom), nullptr, GL_DYNAMIC_DRAW);
//	glBindBuffer(_target, 0);
//
//	return true;
//}
//
//template<GPUSafeStruct Atom>
//void GPUPagedBuffer<Atom>::Destroy() noexcept
//{
//	glDeleteBuffers(1, &m_Name);
//}
//
//template<GPUSafeStruct Atom>
//size_t GPUPagedBuffer<Atom>::UploadPageData(const std::vector<Atom>& data) noexcept
//{
//	const unsigned int count = data.size();
//	glBindBuffer(m_Target, m_Name);
//	glBufferSubData(m_Target, m_AtomCount * sizeof(Atom), count * sizeof(Atom), data.data());
//	glBindBuffer(m_Target, 0);
//
//	m_PageTable.push_back({m_AtomCount, count });
//	m_AtomCount += count;
//	return m_PageTable.size() - 1;
//}
//
//template<GPUSafeStruct Atom>
//bool GPUPagedBuffer<Atom>::UpdatePage(const size_t& pageId, const std::vector<Atom>& data) noexcept
//{
//	const size_t newPageCount = data.size();
//
//	//get page offsets
//	Page page = GetPageOffset(pageId);
//	if (page.IsNull()) {
//		return false;
//	}
//
//	// shift subsequent pages according to new page update
//	const size_t oldNextPageIndex = page.index + page.size;
//	const size_t newNextPageIndex = page.index + newPageCount;
//	const size_t subsequentPagesAtomCount = (m_AtomCount - page.index) + page.size;
//	move(oldNextPageIndex, newNextPageIndex, subsequentPagesAtomCount);
//
//	// update page data on gpu
//	glBindBuffer(m_Target, m_Name);
//	glBufferSubData(m_Target, page.index * sizeof(Atom), newPageCount * sizeof(Atom), data.data());
//	glBindBuffer(m_Target, 0);
//
//	// update m_Buffer state in data structure
//	m_PageTable[pageId].size = newPageCount;
//	const size_t countDelta = newPageCount - page.size;
//	for (int i = pageId + 1; i < m_PageTable.size(); i++)
//	{
//		m_PageTable[i].index += countDelta;
//	}
//
//	m_AtomCount += countDelta;
//	
//	return true;
//}
//
//template<GPUSafeStruct Atom>
//Page GPUPagedBuffer<Atom>::GetPageOffset(const size_t& pageId) noexcept
//{
//	// page does not exisit
//	if (pageId >= m_PageTable.size()) {
//		return Page(0, 0);
//	}
//
//	return m_PageTable[pageId];
//}
//
//template<GPUSafeStruct Atom>
//void GPUPagedBuffer<Atom>::move(size_t srcIndex, size_t dstIndex, size_t length)
//{
//	// if intervials overlap
//	if (srcIndex < (dstIndex + length)
//		&& dstIndex < (srcIndex + length)) {
//
//		// create temp m_Buffer to copy current data into
//		unsigned int copyBuffer;
//		glGenBuffers(1, &copyBuffer);
//		glBindBuffer(GL_COPY_WRITE_BUFFER, copyBuffer);
//		glBufferData(GL_COPY_WRITE_BUFFER, length * sizeof(Atom), nullptr, GL_DYNAMIC_COPY);
//		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
//
//		// copy data into temp copy m_Buffer
//		glBindBuffer(GL_COPY_READ_BUFFER, m_Name);
//		glBindBuffer(GL_COPY_WRITE_BUFFER, copyBuffer);
//		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcIndex * sizeof(Atom), 0, length * sizeof(Atom));
//
//		// copy from temp m_Buffer to move location
//		glBindBuffer(GL_COPY_READ_BUFFER, copyBuffer);
//		glBindBuffer(GL_COPY_WRITE_BUFFER, m_Name);
//		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, dstIndex * sizeof(Atom), length * sizeof(Atom));
//		glBindBuffer(GL_COPY_READ_BUFFER, 0);
//		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
//
//		// delete temp copy m_Buffer
//		glDeleteBuffers(1, &copyBuffer);
//	}
//	else {
//		glBindBuffer(GL_COPY_READ_BUFFER, m_Name);
//		glBindBuffer(GL_COPY_WRITE_BUFFER, m_Name);
//		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcIndex * sizeof(Atom), dstIndex * sizeof(Atom), sizeof(Atom) * length);
//		glBindBuffer(GL_COPY_READ_BUFFER, 0);
//		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
//	}
//}

template<GPUSafeStruct Atom, ThreadMode Mode>
GPUPagedBuffer<Atom, Mode>::GPUPagedBuffer(bool cpuUpdates)
	: m_RawBuffer(cpuUpdates)
{
}

template<GPUSafeStruct Atom, ThreadMode Mode>
GPUPagedBuffer<Atom, Mode>::~GPUPagedBuffer()
{
	Destroy();
}

template<GPUSafeStruct Atom, ThreadMode Mode>
inline Atom* GPUPagedBuffer<Atom, Mode>::operator[](Page pageNum)
{
	assert(pageNum < GetPageCount());
	if constexpr (Mode == ThreadMode::SingleThreaded) assert(IsPageReserved(pageNum));
	return m_RawBuffer.GetContents() + pageNum * m_PageSize;
}

template<GPUSafeStruct Atom, ThreadMode Mode>
const Atom* GPUPagedBuffer<Atom, Mode>::operator[](Page pageNum) const
{
	assert(pageNum < GetPageCount());
	if constexpr (Mode == ThreadMode::SingleThreaded) assert(IsPageReserved(pageNum));
	return m_RawBuffer.GetContents() + pageNum * m_PageSize;
}

template<GPUSafeStruct Atom, ThreadMode Mode>
bool GPUPagedBuffer<Atom, Mode>::Create(GLenum target, size_t pageSize, uint16_t pageCount) noexcept
{
	PROFILE_FUNCTION();

	assert(pageSize > 0 && pageCount > 0);

	bool success = m_RawBuffer.Create(target, pageSize * pageCount);

	if (!success)
	{
		LOG_CRITICAL(EngineSystem::GPU_BUFFER,
			"[GPUPagedBuffer|{}] Failed to create raw buffer",
			m_RawBuffer.GetName());
		return success;
	}

	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUPagedBuffer|{}] Created (pages={}, countPerPage={}, Mode={})",
		m_RawBuffer.GetName(), pageSize, pageCount, GPUAllocatorsUtils::ToString(Mode));

	LOG_DEBUG(EngineSystem::GPU_BUFFER,
		"[GPUPagedBuffer|{}] reserved {:.2f} Mb",
		m_RawBuffer.GetName(),
		((sizeof(Atom) * pageCount * pageSize) / 1000000.0f));

	m_PageSize = pageSize;

	// calculate the number of elements needed for m_FreePages
	const size_t arrSize = (pageCount + WORD_BITS - 1) / WORD_BITS;

	if constexpr (Mode == ThreadMode::SingleThreaded)
	{
		m_FreePages = new uint64_t[arrSize];
		std::fill(m_FreePages, m_FreePages + arrSize, std::numeric_limits<uint64_t>::max());

		// reserve ghost pages in data struct
		if (pageCount % WORD_BITS > 0)
		{
			const uint64_t lastElemUsedPages = pageCount % WORD_BITS;
			const uint64_t ghostPagesMask = ~((1ULL << lastElemUsedPages) - 1);
			m_FreePages[arrSize - 1] &= ~ghostPagesMask;
		}
	}
	else if constexpr (Mode == ThreadMode::LockFree)
	{
		m_FreePages = new std::atomic<uint64_t>[arrSize];

		for (size_t i = 0; i < arrSize; ++i)
			m_FreePages[i].store(std::numeric_limits<uint64_t>::max(), std::memory_order_relaxed);

		if (pageCount % WORD_BITS > 0)
		{
			const uint64_t lastElemUsedPages = pageCount % WORD_BITS;
			const uint64_t ghostPagesMask = ~((1ULL << lastElemUsedPages) - 1);

			uint64_t value = m_FreePages[arrSize - 1].load(std::memory_order_relaxed);
			value &= ~ghostPagesMask;
			m_FreePages[arrSize - 1].store(value, std::memory_order_relaxed);
		}
	}

	return success;
}

template<GPUSafeStruct Atom, ThreadMode Mode>
void GPUPagedBuffer<Atom, Mode>::Destroy() noexcept
{
	PROFILE_FUNCTION();

	LOG_INFO(EngineSystem::GPU_BUFFER,
		"[GPUPagedBuffer|{}] Destroyed",
		m_RawBuffer.GetName());

	m_PageSize = 0;
	if (m_FreePages)
	{
		delete[] m_FreePages;
		m_FreePages = nullptr;
	}

	m_RawBuffer.Destroy();
}

template<GPUSafeStruct Atom, ThreadMode Mode>
void GPUPagedBuffer<Atom, Mode>::ReservePage(Page pageNum)
{
	assert(m_FreePages != nullptr);
	assert(pageNum < GetPageCount());

	const size_t byteIdx = pageNum / WORD_BITS;
	const size_t bitIdx = pageNum % WORD_BITS;


	if constexpr (Mode == ThreadMode::SingleThreaded)
	{
		assert((m_FreePages[byteIdx] & (1ULL << bitIdx)) && "Cannot reserve a reserved page");
		m_FreePages[byteIdx] &= ~(1ULL << bitIdx); // Set bit to 0 => reserved
	}
	else if constexpr (Mode == ThreadMode::LockFree)
	{
		uint64_t mask = (1ULL << bitIdx);

		auto& atom = m_FreePages[byteIdx];

		uint64_t old = atom.load(std::memory_order_relaxed);

		while (true)
		{
			assert(old & mask && "Cannot reserve a reserved page");

			uint64_t desired = old & ~mask;

			if (atom.compare_exchange_weak(
				old,
				desired,
				std::memory_order_acquire,
				std::memory_order_relaxed))
				return;
		}
	}
}

template<GPUSafeStruct Atom, ThreadMode Mode>
void GPUPagedBuffer<Atom, Mode>::FreePage(Page pageNum)
{
	assert(m_FreePages != nullptr);
	assert(pageNum < GetPageCount());

	const size_t byteIdx = pageNum / WORD_BITS;
	const size_t bitIdx = pageNum % WORD_BITS;


	if constexpr (Mode == ThreadMode::SingleThreaded)
	{
		assert((m_FreePages[byteIdx] & (1 << bitIdx)) == 0 && "Cannot free a freed page");
		m_FreePages[byteIdx] |= (1ULL << bitIdx); // Set bit to 1 => free
	}
	else if constexpr (Mode == ThreadMode::LockFree)
	{
		uint64_t mask = (1ULL << bitIdx);

		auto& atom = m_FreePages[byteIdx];

		uint64_t old = atom.load(std::memory_order_relaxed);

		while (true)
		{
			//assert((old & mask) == 0 && "Cannot free a freed page");

			uint64_t desired = old | mask;

			if (atom.compare_exchange_weak(
				old,
				desired,
				std::memory_order_release,
				std::memory_order_relaxed))
				return;
		}
	}
}

template<GPUSafeStruct Atom, ThreadMode Mode>
bool GPUPagedBuffer<Atom, Mode>::ReserveFirstAvaliblePages(unsigned int n, std::vector<uint32_t>& outPages)
{
	assert(m_FreePages != nullptr);
	const size_t pageCount = m_RawBuffer.GetSize() / m_PageSize;
	const size_t freePagesArrSize = (pageCount + WORD_BITS - 1) / WORD_BITS;

	if constexpr (Mode == ThreadMode::SingleThreaded)
	{
		for (size_t index = 0; index < freePagesArrSize; index++)
		{
			uint64_t pagesStatus = m_FreePages[index];
			size_t bitOffset = 0;

			while (pagesStatus != 0)
			{
				unsigned long reserved = GetTrailingZeros(pagesStatus);
				pagesStatus >>= reserved;
				bitOffset += reserved;

				unsigned long free = GetTrailingOnes(pagesStatus);
				unsigned long consume = std::min((unsigned long)n, free);

				uint64_t mask = ((1ULL << consume) - 1ULL) << bitOffset;
				m_FreePages[index] &= ~mask;

				for (unsigned long i = 0; i < consume; i++)
				{
					outPages.push_back(index * WORD_BITS + bitOffset + i);
				}

				n -= consume;

				if (n == 0)
					return true;

				pagesStatus >>= consume;
				bitOffset += consume;
			}
		}
	}
	else if constexpr (Mode == ThreadMode::LockFree)
	{
		for (size_t i = 0; i < freePagesArrSize && n > 0; ++i)
		{
			auto& word = m_FreePages[i];

			while (n > 0)
			{
				uint64_t old = word.load(std::memory_order_relaxed);

				if (old == 0)
					break;

				// Find first free page (bit == 1)
				unsigned long bit = GetTrailingZeros(old);

				uint64_t mask = 1ULL << bit;

				// Somebody already took it
				if ((old & mask) == 0)
					continue;

				uint64_t desired = old & ~mask;

				if (word.compare_exchange_weak(
					old,
					desired,
					std::memory_order_acq_rel,
					std::memory_order_relaxed))
				{
					outPages.push_back(
						static_cast<uint32_t>(i * WORD_BITS + bit));

					--n;
				}
			}
		}
	}

	return n == 0;
}

template<GPUSafeStruct Atom, ThreadMode Mode>
void GPUPagedBuffer<Atom, Mode>::BindBuffer()
{
	m_RawBuffer.BindBuffer();
}

template<GPUSafeStruct Atom, ThreadMode Mode>
bool GPUPagedBuffer<Atom, Mode>::IsPageReserved(Page pageNum) const noexcept
{
	const size_t byteIdx = pageNum / WORD_BITS;
	const size_t bitIdx = pageNum % WORD_BITS;

	if constexpr (Mode == ThreadMode::SingleThreaded)
	{
		// bit == 0 -> reserved
		return (m_FreePages[byteIdx] & (1ULL << bitIdx)) == 0;
	}
	else if constexpr (Mode == ThreadMode::LockFree)
	{
		const size_t word = pageNum / WORD_BITS;
		const size_t bit = pageNum % WORD_BITS;

		return (m_FreePages[word].load(std::memory_order_relaxed) & (1ULL << bit)) == 0;
	}
}

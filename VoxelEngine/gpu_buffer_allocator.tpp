#include "gpu_buffer_allocator.h"
template<typename Atom>
GPUPersistentlyMappedBuffer<Atom>::GPUPersistentlyMappedBuffer(bool _cpuUpdates)
	: m_LockManager(_cpuUpdates)
	, m_BufferContents()
	, m_Name()
	, m_Target()
{}
template<typename Atom>
GPUPersistentlyMappedBuffer<Atom>::~GPUPersistentlyMappedBuffer()
{
	Destroy();
}

template<typename Atom>
bool GPUPersistentlyMappedBuffer<Atom>::Create(GLenum _target, GLuint _count)
{
	if (m_BufferContents) {
		Destroy();
	}

	m_Target = _target;
	m_CountAtoms = _count;
	// This code currently doesn't care about the alignment of the returned memory. This could potentially
	// cause a crash, but since implementations are likely to return us memory that is at lest aligned
	// on a 64-byte boundary we're okay with this for now. 
	// A robust implementation would ensure that the memory returned had enough slop that it could deal
	// with it's own alignment issues, at least. That's more work than I want to do right this second.
	GLbitfield flags = GL_MAP_WRITE_BIT |
		GL_MAP_PERSISTENT_BIT |
		GL_MAP_COHERENT_BIT;

	glGenBuffers(1, &m_Name);
	glBindBuffer(m_Target, m_Name);
	glBufferStorage(m_Target, sizeof(Atom) * _count, nullptr, flags | GL_DYNAMIC_STORAGE_BIT);
	m_BufferContents = reinterpret_cast<Atom*>(glMapBufferRange(m_Target, 0, sizeof(Atom) * _count, flags));

	if (!m_BufferContents) {
		std::cout << "glMapBufferRange failed, probable bug.\n";
		return false;
	}

	return true;
}

template<typename Atom>
void GPUPersistentlyMappedBuffer<Atom>::Destroy()
{
	glBindBuffer(m_Target, m_Name);
	glUnmapBuffer(m_Target);
	glDeleteBuffers(1, &m_Name);

	m_BufferContents = nullptr;
	m_Name = 0;
}

template<typename Atom>
void GPUPersistentlyMappedBuffer<Atom>::WaitForLockedRange(size_t _lockBegin, size_t _lockLength)
{
	m_LockManager.WaitForLockedRange(_lockBegin * sizeof(Atom), _lockLength * sizeof(Atom));
}

template<typename Atom>
void GPUPersistentlyMappedBuffer<Atom>::LockRange(size_t _lockBegin, size_t _lockLength)
{
	m_LockManager.LockRange(_lockBegin * sizeof(Atom), _lockLength * sizeof(Atom));
}

template<typename Atom>
void GPUPersistentlyMappedBuffer<Atom>::BindBuffer()
{
	glBindBuffer(m_Target, m_Name);
}

template<typename Atom>
void GPUPersistentlyMappedBuffer<Atom>::BindBufferBase(GLuint _index)
{
	glBindBufferBase(m_Target, _index, m_Name);
}

template<typename Atom>
void GPUPersistentlyMappedBuffer<Atom>::BindBufferRange(GLuint _index, GLsizeiptr _head, GLsizeiptr _count)
{
	glBindBufferRange(m_Target, _index, m_Name , _head * sizeof(Atom), _count * sizeof(Atom));
}

// ------------------------------------------------------------------------------------------------------------------

template<typename Atom>
GPUCircularBuffer<Atom>::GPUCircularBuffer(bool _cpuUpdates)
	: m_Buffer(_cpuUpdates)
{}

template<typename Atom>
bool GPUCircularBuffer<Atom>::Create(GLenum _target, GLuint _count)
{
	m_Head = 0;
	return m_Buffer.Create(_target, _count);
}

template<typename Atom>
void GPUCircularBuffer<Atom>::Destroy()
{
	m_Buffer.Destroy();
	m_Head = 0;
}

template<typename Atom>
Atom* GPUCircularBuffer<Atom>::Reserve(GLsizeiptr _count)
{
	if (_count > m_Buffer.GetSize()) {
		std::cout << "Requested an update of size " << _count << " for a m_Buffer of size " << m_Buffer.GetSize() << " atoms.\n";
	}

	GLsizeiptr lockStart = m_Head;

	if (lockStart + _count > m_Buffer.GetSize()) {
		// Need to wrap here.
		lockStart = 0;
	}

	m_Buffer.WaitForLockedRange(lockStart, _count);
	return &m_Buffer.GetContents()[lockStart];
}

template<typename Atom>
GLsizeiptr GPUCircularBuffer<Atom>::OnUsageComplete(GLsizeiptr _count)
{
	m_Buffer.LockRange(m_Head, _count);
	GLsizeiptr oldHead = m_Head;
	m_Head = (m_Head + _count) % m_Buffer.GetSize();
	return oldHead;
}

template<typename Atom>
void GPUCircularBuffer<Atom>::BindBuffer()
{
	m_Buffer.BindBuffer();
}

template<typename Atom>
void GPUCircularBuffer<Atom>::BindBufferBase(GLuint _index)
{
	m_Buffer.BindBufferBase(_index);
}


template<typename Atom>
void GPUCircularBuffer<Atom>::BindBufferHeadRange(GLuint _index, GLsizeiptr _count)
{
	m_Buffer.BindBufferRange(_index, m_Head, _count);
}

template<typename Atom>
void GPUCircularBuffer<Atom>::BindBufferRange(GLuint _index, GLsizeiptr _offset, GLsizeiptr _count)
{
	m_Buffer.BindBufferRange(_index, _offset, _count);
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
GPUFixedPagedBuffer<Atom, ObjectID>::GPUFixedPagedBuffer(bool cpuUpdates)
	: m_Buffer(cpuUpdates)
{}

template<typename Atom, typename ObjectID>
GPUFixedPagedBuffer<Atom, ObjectID>::~GPUFixedPagedBuffer()
{
	Destroy();
}

template<typename Atom, typename ObjectID>
bool GPUFixedPagedBuffer<Atom, ObjectID>::Create(GLenum m_Target, size_t pageSize, size_t pageCount) noexcept
{
	if (pageSize == 0 || pageCount == 0)
		return false;

	m_PageSize = pageSize;

	const size_t arrSize = ceil(pageCount / BYTE_TYPE_SIZE);
	m_FreePages = new ByteType[arrSize];
	std::fill(m_FreePages, m_FreePages + arrSize, std::numeric_limits<ByteType>::max());

	return m_Buffer.Create(m_Target, pageSize * pageCount);
}

template<typename Atom, typename ObjectID>
void GPUFixedPagedBuffer<Atom, ObjectID>::Destroy() noexcept
{
	m_PageSize = 0;
	delete[] m_FreePages;
	m_ObjectMapping.clear();

	m_Buffer.Destroy();
}

template<typename Atom, typename ObjectID>
bool GPUFixedPagedBuffer<Atom, ObjectID>::AllocatePages(const ObjectID& obj, unsigned int pages)
{
	assert(pages >= 0);

	if (pages == 0 || m_FreePages == nullptr)
		return false;

	std::vector<unsigned int> allocatedPages = FindFirstFreePages(pages);

	assert(allocatedPages.size() == pages || allocatedPages.empty());

	// Not enough free pages found
	if (allocatedPages.empty())
		return false;

	ReservePages(allocatedPages);

	// Add to object mapping
	GPUObjectAllocation& objAlloc = m_ObjectMapping[obj];
	objAlloc.PushBackPages(allocatedPages);

	return true;
}

template<typename Atom, typename ObjectID>
bool GPUFixedPagedBuffer<Atom, ObjectID>::PushBackToObject(const ObjectID& obj, const Atom& data)
{
	// Check if object has any pages
	if (!m_ObjectMapping.contains(obj))
	{
		if (!AllocatePages(obj, 1))
			return false;
	}

	GPUObjectAllocation& objAlloc = m_ObjectMapping[obj];

	// Calculate the current page index for next insertion
	const unsigned int pageIndex = (objAlloc.count + 1) / m_PageSize;
	const unsigned int pageElemOffset = (objAlloc.count + 1) % m_PageSize;

	// Allocate new page if needed
	if (pageIndex >= objAlloc.GetSize())
	{
		if (!AllocatePages(obj, 1))
			return false;
	}

	Atom* bufferHead = m_Buffer.GetContents();
	const unsigned int targetPage = objAlloc.pages[pageIndex];

	bufferHead[targetPage * m_PageSize + pageElemOffset] = data;
	objAlloc.count++;

	return true;
}

template<typename Atom, typename ObjectID>
void GPUFixedPagedBuffer<Atom, ObjectID>::DeallocateObject(const ObjectID& obj)
{
	if (!m_ObjectMapping.contains(obj))
		return;

	GPUObjectAllocation& alloc = m_ObjectMapping[obj];

	FreePages(alloc.pages);
	m_ObjectMapping.erase(obj);
}

template<typename Atom, typename ObjectID>
std::vector<GPUBufferRange> GPUFixedPagedBuffer<Atom, ObjectID>::GetObjectBufferRanges(const ObjectID& obj) const
{
	std::vector<GPUBufferRange> result;
	if (!m_ObjectMapping.contains(obj))
		return result;

	GPUObjectAllocation& alloc = m_ObjectMapping[obj];
	const unsigned int writePageIdx = alloc.count / m_PageSize;
	const unsigned int writePage = alloc.pages[writePageIdx];

	std::unordered_set<unsigned int> pagesSet;
	pagesSet.insert(alloc.pages.begin(), alloc.pages.begin() + writePageIdx);

	for (const auto& page : pagesSet)
	{	
		// start of buffer range
		if (pagesSet.find(page - 1) == pagesSet.end())
		{
			const unsigned int startPage = page;
			unsigned int endPage = page;

			while (pagesSet.find(page + 1) != pagesSet.end())
				endPage++;

			size_t length = (endPage - startPage) * m_FreePages;
			length -= (endPage == writePage) ? m_PageSize - (alloc.count % m_PageSize) : 0;
			result.emplace_back(GPUBufferRange(startPage * m_PageSize, length));

		}
	}


	return result;
}


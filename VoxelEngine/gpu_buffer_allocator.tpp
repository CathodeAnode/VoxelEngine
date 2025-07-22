#include "gpu_buffer_allocator.h"
template<typename Atom>
GPUPersistentlyMappedBuffer<Atom>::GPUPersistentlyMappedBuffer(bool _cpuUpdates)
	: lockManager(_cpuUpdates)
	, bufferContents()
	, name()
	, target()
{}
template<typename Atom>
GPUPersistentlyMappedBuffer<Atom>::~GPUPersistentlyMappedBuffer()
{
	Destroy();
}

template<typename Atom>
bool GPUPersistentlyMappedBuffer<Atom>::Create(GLenum _target, GLuint _count)
{
	if (bufferContents) {
		Destroy();
	}

	target = _target;
	countAtoms = _count;
	// This code currently doesn't care about the alignment of the returned memory. This could potentially
	// cause a crash, but since implementations are likely to return us memory that is at lest aligned
	// on a 64-byte boundary we're okay with this for now. 
	// A robust implementation would ensure that the memory returned had enough slop that it could deal
	// with it's own alignment issues, at least. That's more work than I want to do right this second.
	GLbitfield flags = GL_MAP_WRITE_BIT |
		GL_MAP_PERSISTENT_BIT |
		GL_MAP_COHERENT_BIT;

	glGenBuffers(1, &name);
	glBindBuffer(target, name);
	glBufferStorage(target, sizeof(Atom) * _count, nullptr, flags | GL_DYNAMIC_STORAGE_BIT);
	bufferContents = reinterpret_cast<Atom*>(glMapBufferRange(target, 0, sizeof(Atom) * _count, flags));

	if (!bufferContents) {
		std::cout << "glMapBufferRange failed, probable bug.\n";
		return false;
	}

	return true;
}

template<typename Atom>
void GPUPersistentlyMappedBuffer<Atom>::Destroy()
{
	glBindBuffer(target, name);
	glUnmapBuffer(target);
	glDeleteBuffers(1, &name);

	bufferContents = nullptr;
	name = 0;
}

template<typename Atom>
void GPUPersistentlyMappedBuffer<Atom>::WaitForLockedRange(size_t _lockBegin, size_t _lockLength)
{
	lockManager.WaitForLockedRange(_lockBegin * sizeof(Atom), _lockLength * sizeof(Atom));
}

template<typename Atom>
void GPUPersistentlyMappedBuffer<Atom>::LockRange(size_t _lockBegin, size_t _lockLength)
{
	lockManager.LockRange(_lockBegin * sizeof(Atom), _lockLength * sizeof(Atom));
}

template<typename Atom>
void GPUPersistentlyMappedBuffer<Atom>::BindBuffer()
{
	glBindBuffer(target, name);
}

template<typename Atom>
void GPUPersistentlyMappedBuffer<Atom>::BindBufferBase(GLuint _index)
{
	glBindBufferBase(target, _index, name);
}

template<typename Atom>
void GPUPersistentlyMappedBuffer<Atom>::BindBufferRange(GLuint _index, GLsizeiptr _head, GLsizeiptr _count)
{
	glBindBufferRange(target, _index, name , _head * sizeof(Atom), _count * sizeof(Atom));
}

// ------------------------------------------------------------------------------------------------------------------

template<typename Atom>
GPUCircularBuffer<Atom>::GPUCircularBuffer(bool _cpuUpdates)
	: buffer(_cpuUpdates)
{}

template<typename Atom>
bool GPUCircularBuffer<Atom>::Create(GLenum _target, GLuint _count)
{
	head = 0;
	return buffer.Create(_target, _count);
}

template<typename Atom>
void GPUCircularBuffer<Atom>::Destroy()
{
	buffer.Destroy();
	head = 0;
}

template<typename Atom>
Atom* GPUCircularBuffer<Atom>::Reserve(GLsizeiptr _count)
{
	if (_count > buffer.GetSize()) {
		std::cout << "Requested an update of size " << _count << " for a buffer of size " << buffer.GetSize() << " atoms.\n";
	}

	GLsizeiptr lockStart = head;

	if (lockStart + _count > buffer.GetSize()) {
		// Need to wrap here.
		lockStart = 0;
	}

	buffer.WaitForLockedRange(lockStart, _count);
	return &buffer.GetContents()[lockStart];
}

template<typename Atom>
GLsizeiptr GPUCircularBuffer<Atom>::OnUsageComplete(GLsizeiptr _count)
{
	buffer.LockRange(head, _count);
	GLsizeiptr oldHead = head;
	head = (head + _count) % buffer.GetSize();
	return oldHead;
}

template<typename Atom>
void GPUCircularBuffer<Atom>::BindBuffer()
{
	buffer.BindBuffer();
}

template<typename Atom>
void GPUCircularBuffer<Atom>::BindBufferBase(GLuint _index)
{
	buffer.BindBufferBase(_index);
}


template<typename Atom>
void GPUCircularBuffer<Atom>::BindBufferHeadRange(GLuint _index, GLsizeiptr _count)
{
	buffer.BindBufferRange(_index, head, _count);
}

template<typename Atom>
void GPUCircularBuffer<Atom>::BindBufferRange(GLuint _index, GLsizeiptr _offset, GLsizeiptr _count)
{
	buffer.BindBufferRange(_index, _offset, _count);
}

// ------------------------------------------------------------------------------------------------------------------

template<typename Atom, typename KeyType>
GPUPagedBuffer<Atom, KeyType>::GPUPagedBuffer()
	: atomCount(0)
	, pageCount(0)
	, maxAtomCount(0)
	, name(0)
	, pendingPageOffsets(kInitialPendingPageOffsetsCapacity)
{}

template<typename Atom, typename KeyType>
GPUPagedBuffer<Atom, KeyType>::~GPUPagedBuffer()
{
	Destroy();
}

template<typename Atom, typename KeyType>
bool GPUPagedBuffer<Atom, KeyType>::Create(GLenum _target, GLuint _count) noexcept
{
	target = _target;
	maxAtomCount = _count;
	
	if (name != 0) return false;

	glGenBuffers(1, &name);
	glBindBuffer(_target, name);
	glBufferData(_target, _count * sizeof(Atom), nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(_target, 0);

	return true;
}

template<typename Atom, typename KeyType>
void GPUPagedBuffer<Atom, KeyType>::Destroy() noexcept
{
	glDeleteBuffers(1, &name);
}

template<typename Atom, typename KeyType>
void GPUPagedBuffer<Atom, KeyType>::UploadPageData(const KeyType& pageKey, const std::vector<Atom>& data) noexcept
{
	const size_t count = data.size();
	glBindBuffer(target, name);
	glBufferSubData(target, atomCount * sizeof(Atom), count * sizeof(Atom), data.data());
	glBindBuffer(target, 0);

	pageCount++;
	pageTable[pageKey] = { pageCount, atomCount, count };
	atomCount += count;

	if (pageCount > pendingPageOffsets.size()) {
		pendingPageOffsets.resize(pendingPageOffsets.size() * 2, 0);
	}
}

template<typename Atom, typename KeyType>
bool GPUPagedBuffer<Atom, KeyType>::UpdatePage(const KeyType& pageKey, const std::vector<Atom>& data) noexcept
{
	const size_t newPageCount = data.size();

	//get page offsets
	Page page = GetPageOffset(pageKey);
	if (page.id == 0) {
		return false;
	}

	// shift subsequent pages according to new page update
	const size_t oldNextPageIndex = page.index + page.size;
	const size_t newNextPageIndex = page.index + newPageCount;
	const size_t subsequentPagesAtomCount = (atomCount - page.index) + page.size;
	move(oldNextPageIndex, newNextPageIndex, subsequentPagesAtomCount);

	// update page data on gpu
	glBindBuffer(target, name);
	glBufferSubData(target, page.index * sizeof(Atom), newPageCount * sizeof(Atom), data.data());
	glBindBuffer(target, 0);

	// update buffer state in data structure
	const size_t countDelta = newPageCount - page.size;
	for (int i = page.id + 1; i <= pageCount; i++)
	{
		pendingPageOffsets[i] += countDelta; 
	}

	atomCount += countDelta;
	pageTable[pageKey].size = newPageCount;
	
	return true;
}

template<typename Atom, typename KeyType>
Page GPUPagedBuffer<Atom, KeyType>::GetPageOffset(const KeyType& pageKey) noexcept
{
	auto it = pageTable.find(pageKey);
	
	// page does not exisit
	if (it == pageTable.end())
	{
		return Page(0, 0, 0);
	}

	Page& page = it->second;
	page.index += pendingPageOffsets[page.id];
	pendingPageOffsets[page.id] = 0; // clear pending offset

	return page;
}

template<typename Atom, typename KeyType>
void GPUPagedBuffer<Atom, KeyType>::move(size_t srcIndex, size_t dstIndex, size_t length)
{
	// if intervials overlap
	if (srcIndex < (dstIndex + length)
		&& dstIndex < (srcIndex + length)) {

		// create temp buffer to copy current data into
		unsigned int copyBuffer;
		glGenBuffers(1, &copyBuffer);
		glBindBuffer(GL_COPY_WRITE_BUFFER, copyBuffer);
		glBufferData(GL_COPY_WRITE_BUFFER, length * sizeof(Atom), nullptr, GL_DYNAMIC_COPY);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

		// copy data into temp copy buffer
		glBindBuffer(GL_COPY_READ_BUFFER, name);
		glBindBuffer(GL_COPY_WRITE_BUFFER, copyBuffer);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcIndex * sizeof(Atom), 0, length * sizeof(Atom));

		// copy from temp buffer to move location
		glBindBuffer(GL_COPY_READ_BUFFER, copyBuffer);
		glBindBuffer(GL_COPY_WRITE_BUFFER, name);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, dstIndex * sizeof(Atom), length * sizeof(Atom));
		glBindBuffer(GL_COPY_READ_BUFFER, 0);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

		// delete temp copy buffer
		glDeleteBuffers(1, &copyBuffer);
	}
	else {
		glBindBuffer(GL_COPY_READ_BUFFER, name);
		glBindBuffer(GL_COPY_WRITE_BUFFER, name);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcIndex * sizeof(Atom), dstIndex * sizeof(Atom), sizeof(Atom) * length);
		glBindBuffer(GL_COPY_READ_BUFFER, 0);
		glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	}
}


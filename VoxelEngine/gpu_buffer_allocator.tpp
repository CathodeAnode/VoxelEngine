#include "gpu_buffer_allocator.h"
template<typename Atom>
GPUBuffer<Atom>::GPUBuffer(bool _cpuUpdates)
	: lockManager(_cpuUpdates)
	, bufferContents()
	, name()
	, target()
	, BufferStorage(BufferStorage::SystemMemory)
{}
template<typename Atom>
GPUBuffer<Atom>::~GPUBuffer()
{
	Destroy();
}

template<typename Atom>
bool GPUBuffer<Atom>::Create(BufferStorage _storage, GLenum _target, GLuint _count, GLbitfield _createFlags, GLbitfield _mapFlags)
{
	if (bufferContents) {
		Destroy();
	}

	bufferStorage = _storage;
	target = _target;

	switch (bufferStorage) {
		case BufferStorage::SystemMemory: {
			bufferContents = new Atom[_count];
			break;
		}

		case BufferStorage::PersistentlyMappedBuffer: {
			// This code currently doesn't care about the alignment of the returned memory. This could potentially
			// cause a crash, but since implementations are likely to return us memory that is at lest aligned
			// on a 64-byte boundary we're okay with this for now. 
			// A robust implementation would ensure that the memory returned had enough slop that it could deal
			// with it's own alignment issues, at least. That's more work than I want to do right this second.

			glGenBuffers(1, &mName);
			glBindBuffer(mTarget, mName);
			glBufferStorage(mTarget, sizeof(Atom) * _count, nullptr, _createFlags);
			mBufferContents = reinterpret_cast<Atom*>(glMapBufferRange(mTarget, 0, sizeof(Atom) * _atomCount, _mapFlags));
			if (!mBufferContents) {
				std::cout << "glMapBufferRange failed, probable bug.\n";
				return false;
			}
			break;
		}

		default: {
			std::cout << "Error: need to update GPUBuffer::Create logic to account for new buffer storage type.\n";
			break;
		}
	};

	return true;
}

template<typename Atom>
void GPUBuffer<Atom>::Destroy()
{
	switch (mBufferStorage) {
		case BufferStorage::SystemMemory: {
			if (bufferContents) delete[] bufferContents;
			break;
		}

		case BufferStorage::PersistentlyMappedBuffer: {
			glBindBuffer(mTarget, mName);
			glUnmapBuffer(mTarget);
			glDeleteBuffers(1, &mName);

			mBufferContents = nullptr;
			mName = 0;
			break;
		}

		default: {
			std::cout << "Error: need to update GPUBuffer::Destroy logic to account for new buffer storage type.\n";
			break;
		}
	};
}

template<typename Atom>
void GPUBuffer<Atom>::WaitForLockedRange(size_t _lockBegin, size_t _lockLength)
{
	lockManager.WaitForLockedRange(_lockBegin, _lockLength);
}

template<typename Atom>
void GPUBuffer<Atom>::LockRange(size_t _lockBegin, size_t _lockLength)
{
	lockManager.LockRange(_lockBegin, _lockLength);
}

template<typename Atom>
void GPUBuffer<Atom>::BindBuffer()
{
	glBindBuffer(target, name);
}

template<typename Atom>
void GPUBuffer<Atom>::BindBufferBase(GLuint _index)
{
	glBindBufferBase(target, _index, name);
}

template<typename Atom>
void GPUBuffer<Atom>::BindBufferRange(GLuint _index, GLsizeiptr _head, GLsizeiptr _count)
{
	glBindBufferRange(target, _index, _head * sizeof(Atom), _count * sizeof(Atom));
}

// ------------------------------------------------------------------------------------------------------------------

template<typename Atom>
GPUCircularBuffer<Atom>::GPUCircularBuffer(bool _cpuUpdates)
	: buffer(_cpuUpdates)
{}

template<typename Atom>
bool GPUCircularBuffer<Atom>::Create(BufferStorage _storage, GLenum _target, GLuint _count, GLbitfield _createFlags, GLbitfield _mapFlags)
{
	head = 0;
	return buffer.Create(_storage, _target, _count, _createFlags, _mapsFlags);
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
	if (_count > buffer.getSize()) {
		std::cout << ("Requested an update of size " << _count << " for a buffer of size " << buffer.getSize() << " atoms.\n";
	}

	GLsizeiptr lockStart = head;

	if (lockStart + _atomCount > buffer.getSize()) {
		// Need to wrap here.
		lockStart = 0;
	}

	buffer.WaitForLockedRange(lockStart, _atomCount);
	return &buffer.GetContents()[lockStart];
}

template<typename Atom>
void GPUCircularBuffer<Atom>::OnUsageComplete(GLsizeiptr _count)
{
	buffer.LockRange(head, _count);
	head = (head + _count) % buffer.getSize();
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
void GPUCircularBuffer<Atom>::BindBufferRange(GLuint _index, GLsizeiptr _count)
{
	buffer.BindBufferRange(_index, head, _count);
}

// ------------------------------------------------------------------------------------------------------------------




#ifndef GPU_BUFFER_LOCK_H
#define GPU_BUFFER_LOCK_H

#include <glad\glad.h>
#include <vector>
#include <iostream>

struct GPUBufferRange 
{
	size_t startOffset;
	size_t length;

	bool Overlaps(const GPUBufferRange& rhs) const {
		return startOffset < (rhs.startOffset + rhs.length)
			&& rhs.startOffset < (startOffset + length);
	}
};

struct GPUBufferLock
{
	GPUBufferRange range;
	GLsync syncObj;
};

class GPUBufferLockManager
{
public:
	GPUBufferLockManager(bool _cpuUpdates);
	~GPUBufferLockManager();

	void WaitForLockedRange(size_t _lockBeginBytes, size_t _lockLength);
	void LockRange(size_t _lockBeginBytes, size_t _lockLength);

private:
	void Wait(GLsync* _syncObj);
	void Cleanup(GPUBufferLock* _bufferLock);

	std::vector<GPUBufferLock> m_BufferLocks;


	// if true, CPU updates, else GPU updates
	bool m_CPUUpdates;

	const uint64_t m_KOneSecondInNanoSeconds = 1000000000;
};

#endif




#include "gpu_buffer_lock.h"

GPUBufferLockManager::GPUBufferLockManager(bool _cpuUpdates)
	: m_CPUUpdates(_cpuUpdates)
{
}

GPUBufferLockManager::~GPUBufferLockManager()
{
    for (auto it = m_BufferLocks.begin(); it != m_BufferLocks.end(); ++it) {
        Cleanup(&*it);
    }

    m_BufferLocks.clear();
}

void GPUBufferLockManager::WaitForLockedRange(size_t _lockBeginBytes, size_t _lockLength)
{
    GPUBufferRange testRange = { _lockBeginBytes, _lockLength };
    std::vector<GPUBufferLock> swapLocks;
    for (auto it = m_BufferLocks.begin(); it != m_BufferLocks.end(); ++it)
    {
        if (testRange.Overlaps(it->range)) {
            Wait(&it->syncObj);
            Cleanup(&*it);
        }
        else {
            swapLocks.push_back(*it);
        }
    }

    m_BufferLocks.swap(swapLocks);
}

void GPUBufferLockManager::LockRange(size_t _lockBeginBytes, size_t _lockLength)
{
    GPUBufferRange newRange = { _lockBeginBytes, _lockLength };
    GLsync syncName = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    GPUBufferLock newLock = { newRange, syncName };

    m_BufferLocks.push_back(newLock);
}

void GPUBufferLockManager::Wait(GLsync* _syncObj)
{
    if (m_CPUUpdates) {
        GLbitfield waitFlags = 0;
        uint64_t waitDuration = 0;
        while (1) {
            GLenum waitRet = glClientWaitSync(*_syncObj, waitFlags, waitDuration);
            if (waitRet == GL_ALREADY_SIGNALED || waitRet == GL_CONDITION_SATISFIED) {
                return;
            }

            if (waitRet == GL_WAIT_FAILED) {
                //assert(!"Not sure what to do here. Probably raise an exception or something.");
                std::cout << "GPU sync wait failed????\n";
                return;
            }

            // After the first time, need to start flushing, and wait for a looong time.
            waitFlags = GL_SYNC_FLUSH_COMMANDS_BIT;
            waitDuration = m_KOneSecondInNanoSeconds;
        }
    }
    else {
        glWaitSync(*_syncObj, 0, GL_TIMEOUT_IGNORED);
    }
}

void GPUBufferLockManager::Cleanup(GPUBufferLock* _bufferLock)
{
    glDeleteSync(_bufferLock->syncObj);
}

#ifndef GPU_HASHMAP_ALLOCATOR_TPP
#define GPU_HASHMAP_ALLOCATOR_TPP

#include "gpu_hashmap_allocator.h"

template<LockFreeKey K, LockFreeValue V, ThreadMode Mode>
GPUHashMap<K, V, Mode>::GPUHashMap()
	: m_Table(true)
{
}

template<LockFreeKey K, LockFreeValue V, ThreadMode Mode>
bool GPUHashMap<K, V, Mode>::Create(size_t cap)
{
    if (!m_Table.Create(GL_SHADER_STORAGE_BUFFER, cap, BufferAccess::ReadWrite))
        return false;

    Entry* table = m_Table.GetContents();

    for (size_t i = 0; i < cap; ++i)
    {
        table[i].key = EMPTY_KEY;
    }

    return true;
}

template<LockFreeKey K, LockFreeValue V, ThreadMode Mode>
void GPUHashMap<K, V, Mode>::Destroy()
{
    m_Table.Destroy();
}

template<LockFreeKey K, LockFreeValue V, ThreadMode Mode>
bool GPUHashMap<K, V, Mode>::Insert(const K& key, const V& value)
{
    Entry* table = m_Table.GetContents();
    size_t cap = m_Table.GetSize();

    uint32_t start = _Hash(key);

    for (size_t probe = 0; probe < cap; ++probe)
    {
        Entry& entry = table[(start + probe) % cap];

        std::atomic_ref<K> atomicKey(entry.key);

        K current = atomicKey.load(std::memory_order_acquire);

        // Empty slot => try to claim
        if (current == EMPTY_KEY || current == TOMBSTONE_KEY)
        {
            K expected = current;

            if (atomicKey.compare_exchange_strong(
                expected,
                key,
                std::memory_order_acq_rel))
            {
                entry.value = value;
                return true;
            }
        }

        // Update existing key
        if (current == key)
        {
            entry.value = value;
            return true;
        }
    }

    return false; // table full
}

template<LockFreeKey K, LockFreeValue V, ThreadMode Mode>
bool GPUHashMap<K, V, Mode>::Find(const K& key, V& out) const
{
    const Entry* table = m_Table.GetContents();
    size_t cap = m_Table.GetSize();

    uint32_t start = _Hash(key);

    for (size_t probe = 0; probe < cap; ++probe)
    {
        const Entry& entry = table[(start + probe) % cap];

        std::atomic_ref<const K> atomicKey(entry.key);

        K current = atomicKey.load(std::memory_order_acquire);

        if (current == EMPTY_KEY)
            return false;

        if (current == key)
        {
            out = entry.value;
            return true;
        }
    }

    return false;
}

template<LockFreeKey K, LockFreeValue V, ThreadMode Mode>
bool GPUHashMap<K, V, Mode>::Contains(const K& key) const
{
    V tmp;
    return Find(key, tmp);
}

template<LockFreeKey K, LockFreeValue V, ThreadMode Mode>
bool GPUHashMap<K, V, Mode>::Erase(const K& key)
{
    Entry* table = m_Table.GetContents();
    size_t cap = m_Table.GetSize();

    uint32_t start = _Hash(key);

    for (size_t probe = 0; probe < cap; ++probe)
    {
        Entry& entry = table[(start + probe) % cap];

        std::atomic_ref<K> atomicKey(entry.key);

        K current = atomicKey.load(std::memory_order_acquire);

        if (current == EMPTY_KEY)
            return false;

        if (current == key)
        {
            K expected = key;

            return atomicKey.compare_exchange_strong(
                expected,
                TOMBSTONE_KEY,
                std::memory_order_acq_rel);
        }
    }

    return false;
}

template<LockFreeKey K, LockFreeValue V, ThreadMode Mode>
void GPUHashMap<K, V, Mode>::BindBuffer(GLuint location)
{
    m_Table.BindBufferBase(location);
}

template<LockFreeKey K, LockFreeValue V, ThreadMode Mode>
template<typename... Args>
bool GPUHashMap<K, V, Mode>::Emplace(const K& key, Args&&... args)
{
    Entry* table = m_Table.GetContents();
    size_t cap = m_Table.GetSize();

    uint32_t start = _Hash(key);

    for (size_t probe = 0; probe < cap; ++probe)
    {
        Entry& entry = table[(start + probe) % cap];

        std::atomic_ref<K> atomicKey(entry.key);

        K current = atomicKey.load(std::memory_order_acquire);

        // Empty slot => try to claim
        if (current == EMPTY_KEY || current == TOMBSTONE_KEY)
        {
            K expected = current;

            if (atomicKey.compare_exchange_strong(
                expected,
                key,
                std::memory_order_acq_rel))
            {
                // construct value in-place
                entry.value = V(std::forward<Args>(args)...);
                return true;
            }
        }

        // Update existing key
        if (current == key)
        {
            entry.value = V(std::forward<Args>(args)...);
            return true;
        }
    }

    return false; // table full
}

#endif
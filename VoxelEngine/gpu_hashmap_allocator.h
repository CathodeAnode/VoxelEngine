#ifndef GPU_HASHMAP_ALLOCATOR_H
#define GPU_HASHMAP_ALLOCATOR_H

#include <atomic>
#include <concepts>
#include <functional>
#include <type_traits>

#include "gpu_buffer_allocator.h"

template<typename T>
concept AtomicCompatible = std::is_trivially_copyable_v<T>;

template<typename T>
concept EqualityComparable = requires(T a, T b) {
        { a == b } -> std::convertible_to<bool>;
};

template<typename K>
concept LockFreeKey = AtomicCompatible<K> && EqualityComparable<K> && std::integral<K>;

template<typename V>
concept LockFreeValue = AtomicCompatible<V>;


template<LockFreeKey K, LockFreeValue V, ThreadMode Mode>
class GPUHashMap
{
public:
    static_assert(Mode != ThreadMode::SingleThreaded, "Single Threaded mode is not implemented");

    GPUHashMap();

    bool Create(size_t cap);
    void Destroy();

    bool Insert(const K& key, const V& value);
    bool Find(const K& key, V& out) const;
    bool Contains(const K& key) const;
    bool Erase(const K& key);

    template<typename... Args>
    bool Emplace(const K& key, Args&&... args);

    void BindBuffer(GLuint location);
private:
    struct Entry 
    {
        K key;
        V value;
    };

    GPUPersistentlyMappedBuffer<Entry> m_Table; // TODO split into two GPUPersistentlyMappedBuffer, one for key and other for value

    static constexpr K EMPTY_KEY = std::numeric_limits<K>::max();;
    static constexpr K TOMBSTONE_KEY = std::numeric_limits<K>::min();;

    static inline uint32_t Hash64(uint64_t x)
    {
        x ^= x >> 33;
        x *= 0xff51afd7ed558ccdULL;
        x ^= x >> 33;
        x *= 0xc4ceb9fe1a85ec53ULL;
        x ^= x >> 33;
        return (uint32_t)x;
    }

    size_t _Hash(const K& key) const
    {
        uint64_t h = Hash64(static_cast<uint64_t>(key));
        return h & (m_Table.GetSize() - 1);
    }

};

#include "gpu_hashmap_allocator.tpp"

#endif


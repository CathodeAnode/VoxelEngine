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
concept Hashable = requires(T a) {
        { std::hash<T>{}(a) } -> std::convertible_to<size_t>;
};

template<typename T>
concept EqualityComparable = requires(T a, T b) {
        { a == b } -> std::convertible_to<bool>;
};

template<typename K>
concept LockFreeKey = AtomicCompatible<K> && Hashable<K> && EqualityComparable<K>;

template<typename V>
concept LockFreeValue = AtomicCompatible<V>;


template<LockFreeKey K, LockFreeValue V>
class GPULockFreeHashMap
{
public:
    GPULockFreeHashMap();

    bool Create(size_t cap);

    bool Insert(const K& key, const V& value);
    bool Find(const K& key, V& out) const;
    bool Contains(const K& key) const;
    bool Erase(const K& key);
private:
    struct Entry 
    {
        K key;
        V value;
    };

    GPUPersistentlyMappedBuffer<Entry> m_Table;

    static constexpr K EMPTY_KEY = K();
    static constexpr K TOMBSTONE_KEY = K(-1);

    size_t _Hash(const K& key) const {
        return std::hash<K>{}(key) % m_Table.GetSize();
    }

};

#include "gpu_hashmap_allocator.tpp"

#endif


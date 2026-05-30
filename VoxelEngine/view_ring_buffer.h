#ifndef __VE_VIEW_RING_BUFFER_H
#define __VE_VIEW_RING_BUFFER_H

#include <cassert>
#include <cstddef>
#include <memory>
#include <ranges>
#include <algorithm>

#include "voxel_math.h"

template<typename T>
class ViewRingBuffer
{
public:
    explicit ViewRingBuffer(size_t capacity)
        : m_Capacity(NextPowerOfTwo(capacity))
        , m_Data(std::make_unique<T[]>(m_Capacity))
    {
        assert(capacity > 0);
    }

    ViewRingBuffer(const ViewRingBuffer&) = delete;
    ViewRingBuffer& operator=(const ViewRingBuffer&) = delete;

    class Range
    {
    public:
        struct iterator
        {
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;
            using iterator_concept = std::forward_iterator_tag;

            T* buffer = nullptr;
            size_t capacity = 0;
            size_t head = 0;
            size_t logicalIndex = 0;

            T& operator*() const
            {
                return buffer[(head + logicalIndex) & (capacity - 1)];
            }

            T* operator->() const
            {
                return &(**this);
            }

            iterator& operator++()
            {
                ++logicalIndex;
                return *this;
            }

            iterator operator++(int)
            {
                iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const iterator& rhs) const
            {
                return logicalIndex == rhs.logicalIndex;
            }

            bool operator!=(const iterator& rhs) const
            {
                return !(*this == rhs);
            }
        };

        iterator begin() const
        {
            return {
                m_Buffer,
                m_Capacity,
                m_Head,
                0
            };
        }

        iterator end() const
        {
            return {
                m_Buffer,
                m_Capacity,
                m_Head,
                m_Count
            };
        }

        inline size_t Size() const
        {
            return m_Count;
        }

    private:
        friend class ViewRingBuffer<T>;

        Range(T* buffer, size_t capacity, size_t head, size_t count)
            : m_Buffer(buffer)
            , m_Capacity(capacity)
            , m_Head(head)
            , m_Count(count)
        {
        }

        T* m_Buffer;
        size_t m_Capacity;
        size_t m_Head;
        size_t m_Count;
    };

    bool Push(const T& value)
    {
        if (m_Tail - m_Head >= m_Capacity)
            return false;

        m_Data[_Index(m_Tail)] = value;

        m_Tail++;
        return true;
    }

    void Unsafe_Push(const T& value)
    {
        m_Data[_Index(m_Tail)] = value;

        m_Tail++;
    }

    bool Pop(T& out)
    {
        if (m_Head == m_Tail)
            return false;

        out = m_Data[_Index(m_Head)];

        m_Head++;
        return true;
    }

    void Consume(size_t count)
    {
        assert(count <= (m_Tail - m_Head));

        m_Head += count;
    }

    T& Front()
    {
        assert(!Empty());
        return m_Data[_Index(m_Head)];
    }

    const T& Front() const
    {
        assert(!Empty());

        return m_Data[_Index(m_Head)];
    }

    Range View(size_t count = SIZE_MAX)
    {
        return Range(
            m_Data.get(),
            m_Capacity,
            _Index(m_Head),
            count);
    }

    inline size_t Size() const
    {
        return m_Tail - m_Head;
    }

    inline bool Empty() const
    {
        return Size() == 0;
    }

    inline size_t Capacity() const
    {
        return m_Capacity;
    }

private:
    size_t _Index(size_t logicalIndex) const
    {
        return logicalIndex & (m_Capacity - 1);
    }

private:
    std::unique_ptr<T[]> m_Data;

    size_t m_Capacity;

    size_t m_Head = 0;
    size_t m_Tail = 0;
};

#endif

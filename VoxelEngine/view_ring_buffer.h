#ifndef __VE_VIEW_RING_BUFFER_H
#define __VE_VIEW_RING_BUFFER_H

#include <cassert>
#include <cstddef>
#include <memory>
#include <ranges>
#include <utility>
#include <algorithm>

#include "voxel_math.h"

template<typename T>
class ViewRingBuffer
{
public:
    explicit ViewRingBuffer(size_t capacity)
        : m_Capacity(NextPowerOfTwo(capacity))
    {
        assert(capacity > 0);

        m_Data = new T[m_Capacity];
    }

    ~ViewRingBuffer()
    {
        if (m_Data)
        {
            delete[] m_Data;
            m_Data = nullptr;
        }
    }

    ViewRingBuffer(const ViewRingBuffer& other)
        : m_Capacity(other.m_Capacity)
        , m_Head(other.m_Head)
        , m_Tail(other.m_Tail)
    {
        m_Data = new T[m_Capacity];
        std::memcpy(m_Data, other.m_Data, m_Capacity * sizeof(T));
    }

    ViewRingBuffer& operator=(const ViewRingBuffer& other)
    {
        if (this != &other)
        {
            delete[] m_Data;

            m_Capacity = other.m_Capacity;
            m_Head = other.m_Head;
            m_Tail = other.m_Tail;

            std::memcpy(m_Data, other.m_Data, m_Capacity * sizeof(T));
        }

        return *this;
    }

    ViewRingBuffer(ViewRingBuffer&& other)
        : m_Capacity(other.m_Capacity)
        , m_Head(other.m_Head)
        , m_Tail(other.m_Tail)
    {
        m_Data = std::move(other.m_Data);

        other.m_Data = nullptr;
        other.m_Capacity = 0;
        other.m_Head = 0;
        other.m_Tail = 0;
    }

    ViewRingBuffer& operator=(ViewRingBuffer&& other)
    {
        if (this != &other)
        {
            delete[] m_Data;

            m_Capacity = other.m_Capacity;
            m_Head = other.m_Head;
            m_Tail = other.m_Tail;
            m_Data = std::move(other.m_Data);

            other.m_Data = nullptr;
            other.m_Capacity = 0;
            other.m_Head = 0;
            other.m_Tail = 0;
        }

        return *this;
    }

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
        assert(m_Capacity > 0);
        assert(m_Data != nullptr);

        if (m_Tail - m_Head >= m_Capacity)
            return false;

        m_Data[_Index(m_Tail)] = value;

        m_Tail++;
        return true;
    }

    bool Push(T&& value)
    {
        assert(m_Capacity > 0);
        assert(m_Data != nullptr);

        if (m_Tail - m_Head >= m_Capacity)
            return false;

        m_Data[_Index(m_Tail)] = std::move(value);

        ++m_Tail;
        return true;
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
        assert(m_Data != nullptr);
        return Range(
            m_Data,
            m_Capacity,
            m_Head,
            std::min(count, Size()));
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
    T* m_Data = nullptr;

    size_t m_Capacity = 0;

    size_t m_Head = 0;
    size_t m_Tail = 0;
};

#endif

#ifndef GPU_CACHE_POLICY_H
#define GPU_CACHE_POLICY_H

#include "gpu_buffer_allocator.h"

#include <concepts>

template<typename Policy, typename ObjectID>
concept EvictionPolicy = requires(Policy policy, const ObjectID & objectID, int cap, GLint bindLocation)
{
    typename Policy::Handle;

    { Policy(cap) } -> std::same_as<Policy>;

    { policy.Create() } -> std::same_as<bool>;
    { policy.OnAccess(std::declval<typename Policy::Handle&>()) } noexcept -> std::same_as<void>;
    { policy.OnInsert(objectID) } noexcept -> std::same_as<typename Policy::Handle>;
    { policy.OnRemove(std::declval<typename Policy::Handle&>()) } noexcept -> std::same_as<void>;
    { policy.SelectVictim() } noexcept -> std::convertible_to<ObjectID>;

    { policy.BindBuffers(bindLocation) } -> std::same_as<void>;
};

template<typename ObjectID>
class LRUPolicy
{
public:
    using Index = uint32_t;
    static constexpr Index NULL_INDEX = std::numeric_limits<Index>::max();

    struct Handle 
    {
        Index index = NULL_INDEX;
    };

public:
    explicit LRUPolicy(int cap)
        : m_FreeList(cap)
        , m_Nodes(cap)
    {}

    // no gpu allocations, thus returns true without executing any code
    bool Create()
    {
        return true;
    }

    void OnAccess(Handle& h) noexcept
    {
        assert(_IsValid(h.index));
        _MoveToFront(h.index);
    }

    Handle OnInsert(const ObjectID& id) noexcept
    {
        Index idx = _AllocateNode();
        Node& n = m_Nodes[idx];

        n.id = id;
        n.prev = NULL_INDEX;
        n.next = NULL_INDEX;

        _InsertFront(idx);

        return Handle{ idx };
    }

    void OnRemove(Handle& h) noexcept
    {
        if (!_IsValid(h.index))
            return;

        _RemoveNode(h.index);
        _FreeNode(h.index);

        h.index = NULL_INDEX;
    }

    [[nodiscard]] ObjectID SelectVictim() noexcept
    {
        assert(m_Tail != NULL_INDEX);
        return m_Nodes[m_Tail].id;
    }

    void BindBuffers(GLint bufferLocation)
    {
        LOG_WARN(EngineSystem::GPU_BUFFER, "LRUPolicy does not have gpu bindings");
    }

private:
    struct Node 
    {
        ObjectID id;
        Index prev = NULL_INDEX;
        Index next = NULL_INDEX;
    };

private:
    std::vector<Node> m_Nodes;
    std::vector<Index> m_FreeList;

    Index m_Head = NULL_INDEX;
    Index m_Tail = NULL_INDEX;

private:
    bool _IsValid(Index i) const noexcept
    {
        return i != NULL_INDEX && i < m_Nodes.size();
    }

    Index _AllocateNode() noexcept
    {
        if (!m_FreeList.empty())
        {
            Index idx = m_FreeList.back();
            m_FreeList.pop_back();
            return idx;
        }

        m_Nodes.emplace_back();
        return static_cast<Index>(m_Nodes.size() - 1);
    }

    void _FreeNode(Index idx) noexcept
    {
        m_FreeList.push_back(idx);
    }

    void _InsertFront(Index idx) noexcept
    {
        Node& n = m_Nodes[idx];

        n.prev = NULL_INDEX;
        n.next = m_Head;

        if (m_Head != NULL_INDEX)
            m_Nodes[m_Head].prev = idx;

        m_Head = idx;

        if (m_Tail == NULL_INDEX)
            m_Tail = idx;
    }

    void _RemoveNode(Index idx) noexcept
    {
        Node& n = m_Nodes[idx];

        if (n.prev != NULL_INDEX)
            m_Nodes[n.prev].next = n.next;
        else
            m_Head = n.next;

        if (n.next != NULL_INDEX)
            m_Nodes[n.next].prev = n.prev;
        else
            m_Tail = n.prev;

        n.prev = NULL_INDEX;
        n.next = NULL_INDEX;
    }

    void _MoveToFront(Index idx) noexcept
    {
        if (idx == m_Head)
            return;

        _RemoveNode(idx);
        _InsertFront(idx);
    }
};

template<typename ObjectID>
class FIFOPolicy
{
public:
    struct Handle
    {
        ObjectID id{};
    };

private:
    struct Slot
    {
        std::atomic<size_t> sequence;
        ObjectID value;
    };

public:
    explicit FIFOPolicy(size_t capacity)
        : m_Capacity(NextPowerOfTwo(capacity)),
        m_Mask(m_Capacity - 1),
        m_Slots(m_Capacity)
    {
        for (size_t i = 0; i < m_Capacity; ++i)
        {
            m_Slots[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    bool Create()
    {
        return true;
    }

    void OnAccess(Handle&) noexcept
    {
        // FIFO ignores accesses
    }

    Handle OnInsert(const ObjectID& objectID) noexcept
    {
        _Enqueue(objectID);
        return Handle{ objectID };
    }

    void OnRemove(Handle&) noexcept
    {
        // nothing required
    }

    ObjectID SelectVictim() noexcept
    {
        ObjectID result{};

        bool success = _Dequeue(result);
        assert(success && "SelectVictim() failed: eviction queue was empty or dequeue operation failed");

        return result;
    }

    void BindBuffers(GLint)
    {
        // no-op
    }

private:
    static size_t NextPowerOfTwo(size_t n)
    {
        size_t p = 1;
        while (p < n)
            p <<= 1;
        return p;
    }

    bool _Enqueue(const ObjectID& value) noexcept
    {
        Slot* slot;
        size_t pos = m_tail.load(std::memory_order_relaxed);

        for (;;)
        {
            slot = &m_Slots[pos & m_Mask];

            size_t seq =
                slot->sequence.load(std::memory_order_acquire);

            intptr_t diff =
                static_cast<intptr_t>(seq) -
                static_cast<intptr_t>(pos);

            if (diff == 0)
            {
                if (m_tail.compare_exchange_weak(
                    pos,
                    pos + 1,
                    std::memory_order_relaxed))
                {
                    break;
                }
            }
            else if (diff < 0)
            {
                // queue full
                return false;
            }
            else
            {
                pos = m_tail.load(std::memory_order_relaxed);
            }
        }

        slot->value = value;

        slot->sequence.store(
            pos + 1,
            std::memory_order_release);

        return true;
    }

    bool _Dequeue(ObjectID& value) noexcept
    {
        Slot* slot;
        size_t pos = m_head.load(std::memory_order_relaxed);

        for (;;)
        {
            slot = &m_Slots[pos & m_Mask];

            size_t seq =
                slot->sequence.load(std::memory_order_acquire);

            intptr_t diff =
                static_cast<intptr_t>(seq) -
                static_cast<intptr_t>(pos + 1);

            if (diff == 0)
            {
                if (m_head.compare_exchange_weak(
                    pos,
                    pos + 1,
                    std::memory_order_relaxed))
                {
                    break;
                }
            }
            else if (diff < 0)
            {
                // queue empty
                return false;
            }
            else
            {
                pos = m_head.load(std::memory_order_relaxed);
            }
        }

        value = slot->value;

        slot->sequence.store(
            pos + m_Capacity,
            std::memory_order_release);

        return true;
    }

private:
    const size_t m_Capacity;
    const size_t m_Mask;

    std::vector<Slot> m_Slots;

    alignas(64) std::atomic<size_t> m_head{ 0 };
    alignas(64) std::atomic<size_t> m_tail{ 0 };
};

#endif
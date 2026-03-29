#ifndef GPU_CACHE_POLICY_H
#define GPU_CACHE_POLICY_H

#include "gpu_buffer_allocator.h"

#include <concepts>

template<typename Policy, typename ObjectID>
concept EvictionPolicy = requires(Policy policy, const ObjectID & objectID, int cap)
{
    typename Policy::Handle;

    { Policy(cap) } -> std::same_as<Policy>;
    { policy.Create() } -> std::same_as<bool>;
    { policy.OnAccess(std::declval<typename Policy::Handle&>()) } noexcept -> std::same_as<void>;
    { policy.OnInsert(objectID) } noexcept -> std::same_as<typename Policy::Handle>;
    { policy.OnRemove(std::declval<typename Policy::Handle&>()) } noexcept -> std::same_as<void>;
    { policy.SelectVictim() } noexcept -> std::convertible_to<ObjectID>;
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
class ClockPolicy
{
public:
    struct Handle
    {
        size_t index;
    };

    explicit ClockPolicy(int cap)
        : m_Capacity(cap)
        , m_ObjectIDs(cap)
        , m_State(true)
    {}

    bool Create()
    {
        bool sucess = m_State.Create(GL_SHADER_STORAGE_BUFFER, m_Capacity, BufferAccess::ReadWrite);
        for (size_t i = 0; i < m_Capacity; ++i)
        {
            std::atomic_ref<uint32_t>(m_State[i]).store(0, std::memory_order_relaxed);
        }
        return sucess;
    }

    void OnAccess(Handle& h) noexcept
    {
        if (h.index < m_Capacity)
        {
            auto state = std::atomic_ref<uint32_t>(m_State[h.index]);
            state.fetch_or(REF_BIT, std::memory_order_relaxed);
        }
    }

    Handle OnInsert(const ObjectID& id) noexcept
    {
        // simple probe starting from thread-local hand
        size_t start = LocalHand();

        for (size_t n = 0; n < m_Capacity; ++n)
        {
            size_t i = (start + n) % m_Capacity;
            auto state = std::atomic_ref<uint32_t>(m_State[i]);

            uint32_t expected = 0;
            if (state.compare_exchange_strong(expected, VALID_BIT | REF_BIT,
                std::memory_order_acq_rel))
            {
                m_ObjectIDs[i] = id;
                return Handle{ i };
            }
        }

        return Handle{};
    }

    void OnRemove(Handle& h) noexcept
    {
        if (h.index < m_Capacity)
        {
            auto state = std::atomic_ref<uint32_t>(m_State[h.index]);
            state.store(0, std::memory_order_release);
        }
    }

    [[nodiscard]] ObjectID SelectVictim() noexcept
    {
        size_t& hand = LocalHand();

        while (true)
        {
            size_t idx = hand;
            hand = (hand + 1) % m_Capacity;

            auto state = std::atomic_ref<uint32_t>(m_State[idx]);
            uint32_t s = state.load(std::memory_order_acquire);

            if (!(s & VALID_BIT))
                continue;

            if (!(s & REF_BIT))
            {
                uint32_t expected = VALID_BIT;
                if (state.compare_exchange_strong(expected, 0,
                    std::memory_order_acq_rel))
                {
                    return m_ObjectIDs[idx];
                }
            }
            else
            {
                // second chance
                state.fetch_and(~REF_BIT, std::memory_order_relaxed);
            }
        }
    }

private:
    static constexpr uint32_t VALID_BIT = 1 << 0;
    static constexpr uint32_t REF_BIT = 1 << 1;

    int m_Capacity;

    // thread-local clock hand
    static size_t& LocalHand() noexcept
    {
        thread_local size_t hand = InitHand();
        return hand;
    }

    static size_t InitHand() noexcept
    {
        // spread threads across the ring (cheap hashing)
        auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
        return tid;
    }

    GPUPersistentlyMappedBuffer<uint32_t> m_State;
    std::vector<ObjectID> m_ObjectIDs;
};

#endif
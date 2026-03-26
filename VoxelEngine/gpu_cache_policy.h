#ifndef GPU_CACHE_POLICY_H
#define GPU_CACHE_POLICY_H

#include <concepts>
#include <list>
#include <unordered_map>

template<typename Policy, typename ObjectID>
concept EvictionPolicy = requires(Policy policy, const ObjectID & objectID)
{
    typename Policy::Handle;

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
    LRUPolicy() = default;

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
    void OnAccess(const ObjectID& id) noexcept;
    void OnInsert(const ObjectID& id) noexcept;
    void OnRemove(const ObjectID& id) noexcept;

    [[nodiscard]] ObjectID SelectVictim() noexcept;
};

#endif
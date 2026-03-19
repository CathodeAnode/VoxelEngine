#ifndef GPU_CACHE_POLICY_H
#define GPU_CACHE_POLICY_H

#include <concepts>
#include <list>
#include <unordered_map>

template<typename Policy, typename ObjectID>
concept EvictionPolicy = requires(Policy policy, const ObjectID & objectID)
{
    { policy.OnAccess(objectID) } noexcept -> std::same_as<void>;
    { policy.OnInsert(objectID) } noexcept -> std::same_as<void>;
    { policy.OnRemove(objectID) } noexcept -> std::same_as<void>;
    { policy.SelectVictim() } noexcept -> std::convertible_to<ObjectID>;
};

template<typename ObjectID>
class LRUPolicy
{
public:
    void OnAccess(const ObjectID& id) noexcept
    {
        auto it = m_ObjectMapping.find(id);
        if (it == m_ObjectMapping.end())
            return;

        m_ObjectAccessHistory.splice(
            m_ObjectAccessHistory.begin(),
            m_ObjectAccessHistory,
            it->second
        );

        it->second = m_ObjectAccessHistory.begin();
    }

    void OnInsert(const ObjectID& id) noexcept
    {
        m_ObjectAccessHistory.push_front(id);
        m_ObjectMapping[id] = m_ObjectAccessHistory.begin();
    }

    void OnRemove(const ObjectID& id) noexcept
    {
        auto it = m_ObjectMapping.find(id);
        if (it == m_ObjectMapping.end())
            return;

        m_ObjectAccessHistory.erase(it->second);
        m_ObjectMapping.erase(it);
    }

    [[nodiscard]] ObjectID SelectVictim() noexcept
    {
        assert(!m_ObjectAccessHistory.empty());
        return m_ObjectAccessHistory.back();
    }

private:
    using ListIt = typename std::list<ObjectID>::iterator;

    std::list<ObjectID> m_ObjectAccessHistory;
    std::unordered_map<ObjectID, ListIt> m_ObjectMapping;
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
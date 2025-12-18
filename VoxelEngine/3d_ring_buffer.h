#ifndef THREE_DIMENSIONAL_RING_BUFFER_H
#define THREE_DIMENSIONAL_RING_BUFFER_H

#include <vector>
#include <cassert>
#include <glm/glm.hpp>

template<typename Datatype_>
class RingBuffer3D
{
public:
    explicit RingBuffer3D(size_t size);

    void Set(const glm::ivec3& coords, const Datatype_& data);

    inline Datatype_& At(const glm::ivec3& coords);
    inline const Datatype_& At(const glm::ivec3& coords) const;

    inline Datatype_& At(int x, int y, int z);
    inline const Datatype_& At(int x, int y, int z) const;

    inline size_t GetLength() const { return m_Length; }

private:
    std::vector<Datatype_> m_RawBuffer;
    size_t m_Length;

    inline size_t _RawIndex(int x, int y, int z) const
    {
        const int offset = static_cast<int>(m_Length / 2);
        const int L = static_cast<int>(m_Length);

        int wx = (((x + offset) % L) + L) % L;
        int wy = (((y + offset) % L) + L) % L;
        int wz = (((z + offset) % L) + L) % L;

        return static_cast<size_t>(wx + wz * L + wy * L * L);
    }
};

template<typename Datatype_>
inline RingBuffer3D<Datatype_>::RingBuffer3D(size_t size)
    : m_RawBuffer(size* size* size)
    , m_Length(size)
{
    assert(m_Length % 2 == 1 && "RingBuffer3D length must be odd to properly center (0,0,0)");
}

template<typename Datatype_>
inline void RingBuffer3D<Datatype_>::Set(const glm::ivec3& coords, const Datatype_& data)
{
    At(coords) = data;
}

template<typename Datatype_>
inline Datatype_& RingBuffer3D<Datatype_>::At(int x, int y, int z)
{
    return m_RawBuffer[_RawIndex(x, y, z)];
}

template<typename Datatype_>
inline const Datatype_& RingBuffer3D<Datatype_>::At(int x, int y, int z) const
{
    return m_RawBuffer[_RawIndex(x, y, z)];
}

template<typename Datatype_>
inline Datatype_& RingBuffer3D<Datatype_>::At(const glm::ivec3& coords)
{
    return At(coords.x, coords.y, coords.z);
}

template<typename Datatype_>
inline const Datatype_& RingBuffer3D<Datatype_>::At(const glm::ivec3& coords) const
{
    return At(coords.x, coords.y, coords.z);
}



#endif

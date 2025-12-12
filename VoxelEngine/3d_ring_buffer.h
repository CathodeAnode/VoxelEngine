#ifndef THREE_DIMENSIONAL_RING_BUFFER_H
#define THREE_DIMENSIONAL_RING_BUFFER_H

#include <vector>
#include <glm/glm.hpp>

template<typename Datatype_>
class RingBuffer3D
{
public:
    explicit RingBuffer3D(size_t size);

    void Set(const glm::ivec3& coords, const Datatype_& data);

    inline Datatype_& At(const glm::ivec3& coords);
    inline Datatype_ At(const glm::ivec3& coords) const;

    inline Datatype_& At(int x, int y, int z);
    inline Datatype_ At(int x, int y, int z) const;

    inline size_t GetLength() const { return m_Length; }

private:
    std::vector<Datatype_> m_RawBuffer;
    size_t m_Length;

    inline size_t _RawIndex(int x, int y, int z) const
    {
        const size_t offset = m_Length / 2;
        int wx = (((x + offset) % m_Length) + m_Length) % m_Length;
        int wy = (((y + offset) % m_Length) + m_Length) % m_Length;
        int wz = (((z + offset) % m_Length) + m_Length) % m_Length;

        return wx + wz * m_Length + wy * m_Length * m_Length;
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
    m_RawBuffer[_RawIndex(coords.x, coords.y, coords.z)] = data;
}

template<typename Datatype_>
inline Datatype_& RingBuffer3D<Datatype_>::At(int x, int y, int z)
{
    return m_RawBuffer[_RawIndex(x, y, z)];
}

template<typename Datatype_>
inline Datatype_ RingBuffer3D<Datatype_>::At(int x, int y, int z) const
{
    return m_RawBuffer[_RawIndex(x, y, z)];
}

template<typename Datatype_>
inline Datatype_& RingBuffer3D<Datatype_>::At(const glm::ivec3& coords)
{
    return At(coords.x, coords.y, coords.z);
}

template<typename Datatype_>
inline Datatype_ RingBuffer3D<Datatype_>::At(const glm::ivec3& coords) const
{
    return At(coords.x, coords.y, coords.z);
}



#endif

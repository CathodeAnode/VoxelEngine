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
    glm::ivec3 m_Offset;

    inline size_t Index(int x, int y, int z) const
    {
        auto wrap = [&](int v) {
            v %= int(m_Length);
            return (v < 0) ? v + m_Length : v;
            };

        int wx = wrap(x + m_Offset.x);
        int wy = wrap(y + m_Offset.y);
        int wz = wrap(z + m_Offset.z);

        return wx + wz * m_Length + wy * m_Length * m_Length;
    }
};

template<typename Datatype_>
inline RingBuffer3D<Datatype_>::RingBuffer3D(size_t size)
    : m_RawBuffer(size* size* size)
    , m_Length(size)
    , m_Offset(int(size / 2), int(size / 2), int(size / 2))
{}

template<typename Datatype_>
inline void RingBuffer3D<Datatype_>::Set(const glm::ivec3& coords, const Datatype_& data)
{
    m_RawBuffer[Index(coords.x, coords.y, coords.z)] = data;
}

template<typename Datatype_>
inline Datatype_& RingBuffer3D<Datatype_>::At(int x, int y, int z)
{
    return m_RawBuffer[Index(x, y, z)];
}

template<typename Datatype_>
inline Datatype_ RingBuffer3D<Datatype_>::At(int x, int y, int z) const
{
    return m_RawBuffer[Index(x, y, z)];
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

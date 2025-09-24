#ifndef THREE_DIMENSIONAL_RING_BUFFER_H
#define THREE_DIMENSIONAL_RING_BUFFER_H

#include <glm/glm.hpp>

#include <vector>

template<typename Datatype_>
class RingBuffer3D
{
public:
	explicit RingBuffer3D(size_t size);

	void Set(const glm::ivec3& coords, const Datatype_& data);

	inline Datatype_& At(const glm::ivec3& coords);
	inline Datatype_ At(const glm::ivec3& coords) const;

private:
	std::vector<Datatype_> m_RawBuffer;
	size_t m_Length;
};


template<typename Datatype_>
inline RingBuffer3D<Datatype_>::RingBuffer3D(size_t size)
	: m_RawBuffer(size * size * size)
	, m_Length(size)
{}

template<typename Datatype_>
inline void RingBuffer3D<Datatype_>::Set(const glm::ivec3 & coords, const Datatype_ & data)
{
	buffer[coords.x + coords.z * m_Length + coords.y * m_Length * m_Length] = data;
}


template<typename Datatype_>
inline Datatype_& RingBuffer3D<Datatype_>::At(const glm::ivec3& coords)
{
	return buffer[coords.x + coords.z * m_Length + coords.y * m_Length * m_Length];
}

template<typename Datatype_>
inline Datatype_ RingBuffer3D<Datatype_>::At(const glm::ivec3& coords) const
{
	return buffer[coords.x + coords.z * m_Length + coords.y * m_Length * m_Length];
}

#endif



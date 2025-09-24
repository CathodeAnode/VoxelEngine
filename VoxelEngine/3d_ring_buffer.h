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

	inline size_t GetLength() { return m_Length; }

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
	glm::ivec3 index(
		coords.x % m_Length,
		coords.y % m_Length,
		coords.z % m_Length
	);

	buffer[index.x + index.z * m_Length + index.y * m_Length * m_Length] = data;
}


template<typename Datatype_>
inline Datatype_& RingBuffer3D<Datatype_>::At(const glm::ivec3& coords)
{
	glm::ivec3 index(
		coords.x % m_Length,
		coords.y % m_Length,
		coords.z % m_Length
	);

	return buffer[index.x + index.z * m_Length + index.y * m_Length * m_Length];
}

template<typename Datatype_>
inline Datatype_ RingBuffer3D<Datatype_>::At(const glm::ivec3& coords) const
{
	glm::ivec3 index(
		coords.x % m_Length,
		coords.y % m_Length,
		coords.z % m_Length
	);

	return buffer[index.x + index.z * m_Length + index.y * m_Length * m_Length];
}

#endif



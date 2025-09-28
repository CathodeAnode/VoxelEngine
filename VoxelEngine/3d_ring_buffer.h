#ifndef THREE_DIMENSIONAL_RING_BUFFER_H
#define THREE_DIMENSIONAL_RING_BUFFER_H

#include <vector>

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

	m_RawBuffer[index.x + index.z * m_Length + index.y * m_Length * m_Length] = data;
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

template<typename Datatype_>
inline Datatype_& RingBuffer3D<Datatype_>::At(int x, int y, int z)
{
	return m_RawBuffer[x % m_Length + (z * m_Length) % m_Length + (y * m_Length * m_Length) % m_Length];
}

template<typename Datatype_>
inline Datatype_ RingBuffer3D<Datatype_>::At(int x, int y, int z) const
{
	return m_RawBuffer[x % m_Length + (z * m_Length) % m_Length + (y * m_Length * m_Length) % m_Length];
}

#endif



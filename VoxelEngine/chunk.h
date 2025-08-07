#ifndef CHUNK_H
#define CHUNK_H

#include <cstring>
#include <iostream>
#include <unordered_map>
#include <limits>


template<typename T, size_t ChunkSize>
class Chunk {
public:
	using ValueType = T;
	static constexpr unsigned int Size = ChunkSize;

	Chunk(bool filled=false) {
		m_OpaqueData = new T[ChunkSize * ChunkSize];
		if (filled) {
			std::fill(m_OpaqueData, m_OpaqueData + ChunkSize * ChunkSize, ~T(0));  // set all bits to 1
			for (int i = 0; i < ChunkSize * ChunkSize * ChunkSize; i++) {
				m_VoxelData[i] = 0x000000808080; // grey color
			}
			//toggleBit(4, 7, 4);
		}
		
	};

	// Move constructor
	Chunk(Chunk&& other) noexcept {
		delete[] m_OpaqueData;
		m_OpaqueData = std::move(other.m_OpaqueData);
		m_VoxelData = std::move(other.m_VoxelData);
		other.m_OpaqueData = nullptr;
	}

	// Copy constructor
	Chunk(const Chunk& other) {
		m_OpaqueData = new T[ChunkSize * ChunkSize];
		std::memcpy(m_OpaqueData, other.m_OpaqueData, ChunkSize * ChunkSize * sizeof(T));
		m_VoxelData = other.m_VoxelData;
	}

	// copy assignment
	Chunk& operator=(const Chunk& other) {
		if (this != &other) {
			delete[] m_OpaqueData;
			m_OpaqueData = new T[ChunkSize * ChunkSize];
			std::memcpy(m_OpaqueData, other.m_OpaqueData, ChunkSize * ChunkSize * sizeof(T));
			m_VoxelData = other.m_VoxelData;
		}
		return *this;
	}

	// Move assignment
	Chunk& operator=(Chunk&& other) noexcept {
		if (this != &other) {
			delete[] m_OpaqueData;
			m_OpaqueData = other.m_OpaqueData;
			m_VoxelData = std::move(other.m_VoxelData);
			other.m_OpaqueData = nullptr;
		}
		return *this;
	}

	~Chunk() {
		if (m_OpaqueData) {
			delete[] m_OpaqueData;
		}
	};
	
	bool isSolid(int x, int y, int z) const {
		int rowIndex = x + y * ChunkSize;
		return m_OpaqueData[rowIndex] << z;
	}

	bool isEmpty() const {
		for (int i = 0; i < ChunkSize * ChunkSize; i++) {
			if (m_OpaqueData[i] != 0) {
				return false;
			}
		}
		return true;
	}

	uint16_t getVoxelData(int x, int y, int z) {
		if (m_VoxelData.contains(x + y * ChunkSize + z * ChunkSize * ChunkSize)) {
			return m_VoxelData.at(x + y * ChunkSize + z * ChunkSize * ChunkSize);
		}

		return std::numeric_limits<uint16_t>::max();
	}

	uint16_t getVoxelData(glm::ivec3 coords) {
		return getVoxelData(coords.x, coords.y, coords.z);
	}

	T getColumnRow(int x, int y) {
		return m_OpaqueData[x + (y * ChunkSize)];
	}

	T getColumnRow(int index) {
		return m_OpaqueData[index];
	}

	// Toggle the x, y, z-th bit in the m_OpaqueData
	void toggleBit(int x, int y, int z) {
		int index = x + z * ChunkSize;
		m_OpaqueData[index] ^= (1u << y);  // Toggle the z-th bit using XOR
	}

	void printData() {

		unsigned int bits_per_element = sizeof(T) * 8;

		for (unsigned int y = 0; y < ChunkSize; ++y) {
			for (unsigned int x = 0; x < ChunkSize; ++x) {
				T val = m_OpaqueData[x + y * ChunkSize];
				// print bits in val from most significant to least significant
				for (int bit = bits_per_element - 1; bit >= 0; --bit) {
					bool bit_set = (val & (T(1) << bit)) != 0;
					std::cout << (bit_set ? '1' : '0');
				}
				std::cout << ' ';  // space between elements for clarity
			}
			std::cout << std::endl;
		}
	}
	//static int coordsToDataLoc(int x, int y, int z);

	char* serialize();
private:
	T* m_OpaqueData = nullptr; // 1 for block, 0 for air (z-major order)
	/* maps block indes to a rgb color or texture, air blocks dont have a mapping
	 * if msb is set to 0, block is colored with first 8 bits for rgb, other 8 bits ignored
	 * if msb is set to 1, block is textured with texture id as uint16_t (max textures: 32,768)*/
	std::unordered_map<unsigned int, uint16_t> m_VoxelData; 
};

typedef Chunk<uint8_t, 8> Chunk8;
typedef Chunk<uint16_t, 16> Chunk16;
typedef Chunk<uint32_t, 32> Chunk32;


#endif // !CHUNK_H

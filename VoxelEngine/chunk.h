#ifndef CHUNK_H
#define CHUNK_H

#include <cstring>
#include <iostream>
#include <unordered_map>
#include <limits>


template<typename T, unsigned int ChunkSize> class Chunk {
private:
	T* opaqueData = nullptr; // 1 for block, 0 for air (z-major order)
	/* maps block indes to a rgb color or texture, air blocks dont have a mapping
	 * if msb is set to 0, block is colored with first 8 bits for rgb, other 8 bits ignored
	 * if msb is set to 1, block is textured with texture id as uint16_t (max textures: 32,768)*/
	std::unordered_map<unsigned int, uint16_t> voxelData; 

public:
	using ValueType = T;
	static constexpr unsigned int Size = ChunkSize;

	Chunk(bool filled=false) {
		opaqueData = new T[ChunkSize * ChunkSize];
		if (filled) {
			std::fill(opaqueData, opaqueData + ChunkSize * ChunkSize, ~T(0));  // set all bits to 1
			for (int i = 0; i < ChunkSize * ChunkSize * ChunkSize; i++) {
				voxelData[i] = 0x000000808080; // grey color
			}
			//toggleBit(4, 7, 4);
		}
		
	};

	// Move constructor
	Chunk(Chunk&& other) noexcept {
		opaqueData = other.opaqueData;
		voxelData = std::move(other.voxelData);
		other.opaqueData = nullptr;
	}

	// Copy constructor
	Chunk(const Chunk& other) {
		opaqueData = new T[ChunkSize * ChunkSize];
		std::memcpy(opaqueData, other.opaqueData, ChunkSize * ChunkSize * sizeof(T));
		voxelData = other.voxelData;
	}

	// copy assignment
	Chunk& operator=(const Chunk& other) {
		if (this != &other) {
			delete[] opaqueData;
			opaqueData = new T[ChunkSize * ChunkSize];
			std::memcpy(opaqueData, other.opaqueData, ChunkSize * ChunkSize * sizeof(T));
			voxelData = other.voxelData;
		}
		return *this;
	}

	// Move assignment
	Chunk& operator=(Chunk&& other) noexcept {
		if (this != &other) {
			delete[] opaqueData;
			opaqueData = other.opaqueData;
			voxelData = std::move(other.voxelData);
			other.opaqueData = nullptr;
		}
		return *this;
	}

	~Chunk() {
		if (opaqueData) {
			delete[] opaqueData;
		}
	};
	
	bool isSolid(int x, int y, int z) const {
		int rowIndex = x + y * ChunkSize;
		return opaqueData[rowIndex] << z;
	}

	bool isEmpty() const {
		for (int i = 0; i < Size * Size; i++) {
			if (opaqueData[i] != 0) {
				return false;
			}
		}
		return true;
	}

	uint16_t getVoxelData(int x, int y, int z) {
		if (voxelData.contains(x + y * ChunkSize + z * ChunkSize * ChunkSize)) {
			return voxelData.at(x + y * ChunkSize + z * ChunkSize * ChunkSize);
		}

		return std::numeric_limits<uint16_t>::max();
	}

	uint16_t getVoxelData(glm::ivec3 coords) {
		return getVoxelData(coords.x, coords.y, coords.z);
	}

	T getColumnRow(int x, int y) {
		return opaqueData[x + (y * ChunkSize)];
	}

	T getColumnRow(int index) {
		return opaqueData[index];
	}

	// Toggle the x, y, z-th bit in the opaqueData
	void toggleBit(int x, int y, int z) {
		int index = x + z * ChunkSize;
		opaqueData[index] ^= (1u << y);  // Toggle the z-th bit using XOR
	}

	void printData() {

		unsigned int bits_per_element = sizeof(T) * 8;

		for (unsigned int y = 0; y < ChunkSize; ++y) {
			for (unsigned int x = 0; x < ChunkSize; ++x) {
				T val = opaqueData[x + y * ChunkSize];
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

};

typedef Chunk<uint8_t, 8> Chunk8;
typedef Chunk<uint16_t, 16> Chunk16;
typedef Chunk<uint32_t, 32> Chunk32;


#endif // !CHUNK_H

#include "chunk.h"

Chunk::Chunk() {
    data.flip();
}

Chunk::~Chunk() {
}


void Chunk::toggleBlock(int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0) {
        throw std::invalid_argument("Chunk coordinates must be greater than zero");
    }

    if (x >= CHUNK_SIZE || y >= CHUNK_SIZE || z >= CHUNK_SIZE) {
        throw std::invalid_argument("Chunk coordinates must be less than chunk size");
    }

    int bitIndex = x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;

    data.flip(bitIndex);
}

void Chunk::printData() const {
    for (int z = 0; z < CHUNK_SIZE; ++z) {
        std::cout << "Layer z = " << z << ":\n";
        for (int y = 0; y < CHUNK_SIZE; ++y) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                int index = x + CHUNK_SIZE * (y + CHUNK_SIZE * z);
                std::cout << (data[index] ? '1' : '0');
            }
            std::cout << '\n';
        }
        std::cout << '\n';
    }
}

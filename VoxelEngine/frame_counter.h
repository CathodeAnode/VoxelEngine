#ifndef FRAMECOUNTER_H
#define FRAMECOUNTER_H

#include <cstdint>

template<typename ChunkT>
class Application;

class FrameCounter
{

public:
	inline static uint64_t Get() { return s_frame; }

private:
	inline static uint64_t s_frame = 0;

	inline static void Tick() { s_frame++; }

private:
	template<typename ChunkT>
	friend class Application;
};

#endif


#ifndef UID_MANAGER_H
#define UID_MANAGER_H

#include <atomic>
#include "types.h"

class UIDManager
{
public:

	static inline VoxelObjectID Generate() { return ++s_UID; }
private:
	static inline std::atomic<VoxelObjectID> s_UID{0};
};


#endif


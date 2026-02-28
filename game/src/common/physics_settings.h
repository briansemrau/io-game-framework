#ifndef PHYSICS_SETTINGS_H
#define PHYSICS_SETTINGS_H

#include <cstdint>

enum class CollisionBits : uint64_t {
	StaticBit = 0x0001,
	CarBit = 0x0002,
	MoverBit = 0x0004,
	DynamicBit = 0x0008,
	DebrisBit = 0x0010,

	AllBits = ~0u,
};

#endif // PHYSICS_SETTINGS_H

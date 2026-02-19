#ifndef PHYSICS_H
#define PHYSICS_H

#include <cstdint>

// TODO does box2d v3 have a recommended physics user customization header?

static constexpr float pixelsPerMeter = 64.0f;

enum CollisionBits : uint64_t
{
	StaticBit = 0x0001,
	MoverBit = 0x0002,
	DynamicBit = 0x0004,
	DebrisBit = 0x0008,

	AllBits = ~0u,
};

#endif // PHYSICS_H

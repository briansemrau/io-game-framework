#include "raylib.h"
#include "box2d/box2d.h"
#include <iostream>
#include <format>
#include <assert.h>
#include <algorithm>
#include <type_traits>

#include "physics_settings.h"

template<class F, class... Args>
concept VoidCallable = std::invocable<F, Args...> && std::same_as<std::invoke_result_t<F, Args...>, void>;

struct Slot {
    int priority;
    F *fcn;

    auto cmp(const Slot<F> &a, const Slot<F> &b) {
        return (a.priority != b.priority) ? a.priority < b.priority : std::less<F>{}(a.fcn, b.fcn);
    }
};

template<typename T>
struct Slots {
    std::vector<Slot<T>> slotVec;

    auto addSlot(T fcn, int priority = 0) {
        auto it = std::lower_bound(slotVec.begin(), slotVec.end(), fcn, Slot<T>::cmp);
        slotVec.insert(it, {priority, fcn});
    }
};

void initCommon() {
    // Create Box2D world
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 9.81f};
    b2WorldId worldId = b2CreateWorld(&worldDef);
    b2DebugDraw debugDraw = b2RaylibDebugDraw();
    debugDraw.drawBounds = true;
    debugDraw.drawBodyNames = true;
    debugDraw.drawShapes = true;
    debugDraw.drawJoints = true;
    debugDraw.drawContactPoints = true;
    debugDraw.drawIslands = true;

    Slots<b2PreSolveFcn*> preSolveSlots;
    b2World_SetPreSolveCallback(worldId, [](b2ShapeId shapeIdA, b2ShapeId shapeIdB, b2Vec2 point, b2Vec2 normal, void*) -> bool {

    }, nullptr);
    
    // initialize stuff

    // TODO move all of the above into a game state abstraction
    // TODO figure out a good input state abstraction with contextual data (not just button input)
    // TODO move rendering to a separate rendering layer that depends on the game state abstraction layer
    // game state is the foundational layer.
    // input state operates on the interface provided by game state
    // renderer retrieves data using the interface provided by game state

    

}

void stepCommon() {
    // Update physics
    float deltaTime = GetFrameTime();
    b2World_Step(worldId, deltaTime, 4);
}
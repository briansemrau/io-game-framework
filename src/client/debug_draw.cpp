#include "debug_draw.h"

#include "raylib.h"
#include "physics_settings.h"

#include <vector>

// this c++ function decl syntax scares me
auto colorConv(b2HexColor color) -> Color {
    return {(unsigned char)(color >> 16 & 0xFF), (unsigned char)(color >> 8 & 0xFF), (unsigned char)(color & 0xFF), 0xFF};
}

auto b2VecToVector2(const b2Vec2& v) -> Vector2 {
    return Vector2(v.x * pixelsPerMeter, v.y * pixelsPerMeter);
}

b2DebugDraw b2RaylibDebugDraw() {
    b2DebugDraw debugDraw = b2DefaultDebugDraw();
    debugDraw.DrawPolygonFcn = [](const b2Vec2* vertices, int vertexCount, b2HexColor color, void*) {
        std::vector<Vector2> v;
        v.resize(vertexCount+1);
        for (size_t i = 0; i < vertexCount; ++i) {
            v[i] = b2VecToVector2(vertices[vertexCount - i - 1]);
        }
        v[vertexCount] = v[0];
        DrawLineStrip(v.data(), vertexCount, colorConv(color));
    };
    debugDraw.DrawSolidPolygonFcn = [](b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor color, void*) {
        std::vector<Vector2> v;
        v.resize(vertexCount);
        for (size_t i = 0; i < vertexCount; ++i) {
            v[i] = b2VecToVector2(b2TransformPoint(transform, vertices[vertexCount - i - 1]));
        }
        DrawTriangleFan(v.data(), vertexCount, colorConv(color));
    };
    debugDraw.DrawCircleFcn = [](b2Vec2 center, float radius, b2HexColor color, void*) {
        DrawCircleLinesV(b2VecToVector2(center), radius * pixelsPerMeter, colorConv(color));
    };
    debugDraw.DrawSolidCircleFcn = [](b2Transform transform, float radius, b2HexColor color, void*) {
        DrawCircleV(b2VecToVector2(transform.p), radius * pixelsPerMeter, colorConv(color));
    };
    debugDraw.DrawSolidCapsuleFcn = [](b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void*) {
        const int seg = std::max(4, (int)(radius * pixelsPerMeter * 3));
        const float step = PI / seg;
        b2Vec2 d = p2 - p1;
        b2Normalize(d);
        b2Vec2 n(-d.y, d.x);
        b2Vec2 c = 0.5f * (p1 + p2);

        std::vector<Vector2> verts;
        verts.reserve(2 + 2 * seg);
        verts.push_back(b2VecToVector2(p1 + n * radius));
        verts.push_back(b2VecToVector2(p2 + n * radius));
        for (int i = 1; i < seg; ++i) {
            float a = PI * 0.5f - i * step;
            float cs = cosf(a);
            float sn = sinf(a);
            b2Vec2 off = d * cs + n * sn;
            verts.push_back(b2VecToVector2(p2 + off * radius));
        }
        verts.push_back(b2VecToVector2(p2 - n * radius));
        verts.push_back(b2VecToVector2(p1 - n * radius));
        for (int i = 1; i < seg; ++i) {
            float a = PI * -0.5f - i * step;
            float cs = cosf(a);
            float sn = sinf(a);
            b2Vec2 off = d * cs + n * sn;
            verts.push_back(b2VecToVector2(p1 + off * radius));
        }
        DrawTriangleFan(verts.data(), verts.size(), colorConv(color));
    };
    debugDraw.DrawLineFcn = [](b2Vec2 p1, b2Vec2 p2, b2HexColor color, void*) {
        DrawLineV(b2VecToVector2(p1), b2VecToVector2(p2), colorConv(color));
    };
    debugDraw.DrawTransformFcn = [](b2Transform transform, void*) {
        b2Vec2 p1 = transform.p;
        b2Vec2 p2 = b2MulAdd(p1, 1.0f, b2Rot_GetXAxis(transform.q));
        DrawLineV(b2VecToVector2(p1), b2VecToVector2(p2), RED);
        p2 = b2MulAdd(p1, 1.0f, b2Rot_GetYAxis(transform.q));
        DrawLineV(b2VecToVector2(p1), b2VecToVector2(p2), GREEN);
    };
    debugDraw.DrawPointFcn = [](b2Vec2 p, float size, b2HexColor color, void*) {
        DrawCircleV(b2VecToVector2(p), size * pixelsPerMeter, colorConv(color));
    };
    debugDraw.DrawStringFcn = [](b2Vec2 p, const char* s, b2HexColor color, void*) {
        DrawText(s, p.x * pixelsPerMeter, p.y * pixelsPerMeter, 20, colorConv(color));
    };
    return debugDraw;
}

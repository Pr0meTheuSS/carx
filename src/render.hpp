#pragma once
#include "raylib.h"

class Renderer {
public:
    void begin3D(const Camera3D& camera) {
        BeginMode3D(camera);
    }

    void end3D() {
        EndMode3D();
    }
};
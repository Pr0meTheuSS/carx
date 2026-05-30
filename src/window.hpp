#pragma once
#include "raylib.h"

class Window {
   public:
    Window(int w, int h, const char* title) { InitWindow(w, h, title); }

    ~Window() { CloseWindow(); }

    bool shouldClose() const { return WindowShouldClose(); }

    void beginFrame() {
        BeginDrawing();
        ClearBackground(RAYWHITE);
    }

    void endFrame() { EndDrawing(); }
};
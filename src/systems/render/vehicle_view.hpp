#pragma once
#include "core/track.hpp"
#include "raylib.h"
#include "renderable.hpp"

class VehicleView final : public Renderable {
   public:
    VehicleView(const Vehicle& vehicle) : mVehicle(vehicle) {}

    void render() const override {
        auto& t = mTransform;

        // корпус
        DrawCubeV(t.translation, {2, 0.5f, 4}, RED);
        DrawCubeWiresV(t.translation, {2, 0.5f, 4}, BLACK);

        // колёса (упрощённо)
        float offsetX = 0.9f;
        float offsetZ = 1.5f;

        Vector3 offsets[4] = {{offsetX, -0.5f, offsetZ}, {-offsetX, -0.5f, offsetZ}, {offsetX, -0.5f, -offsetZ}, {-offsetX, -0.5f, -offsetZ}};
        for (int i = 0; i < 4; i++) {
            Vector3 position{t.translation.x + offsets[i].x, t.translation.y + offsets[i].y, t.translation.z + offsets[i].z};
            DrawSphere(position, 0.4f, DARKGRAY);
        }
    }

   private:
    const Vehicle& mVehicle;
};
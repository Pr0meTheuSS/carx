#pragma once
#include <memory>

#include "systems/render/scene.hpp"
#include "core/vehicle.hpp"
#include "core/track.hpp"
#include "systems/render/vehicle_view.hpp"
#include "systems/render/track_view.hpp"
#include "window.hpp"
#include "render.hpp"

class App {
public:
    App()
        : window(800, 600, "Car"),
          camera{}
    {
        camera.position = {6, 6, 6};
        camera.target = {0, 0, 0};
        camera.up = {0, 1, 0};
        camera.fovy = 45;

        vehicle = std::make_unique<Vehicle>();
        track   = std::make_unique<Track>(1.0, 1);

        vehicleView = std::make_shared<VehicleView>(*vehicle);
        trackView   = std::make_shared<TrackView>(*track);

        vehicleView->transform().translation = {0, 0.5f, 0};

        scene.addChild(vehicleView);
        scene.addChild(trackView);
    }

    void run() {
        SetTargetFPS(60);

        while (!window.shouldClose()) {

            update();
            render();
        }
    }

private:
    void update() {
        // future physics/game logic
        // vehicle->update(dt);
    }

    void render() {
        window.beginFrame();

        renderer.begin3D(camera);

        scene.render();

        renderer.end3D();

        window.endFrame();
    }

private:
    Window window;
    Renderer renderer;
    Camera3D camera;

    Scene scene;

    std::unique_ptr<Vehicle> vehicle;
    std::unique_ptr<Track> track;

    std::shared_ptr<VehicleView> vehicleView;
    std::shared_ptr<TrackView> trackView;
};
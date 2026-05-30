#pragma once
#include "../../core/track.hpp"
#include "raylib.h"
#include "renderable.hpp"

class TrackView final : public Renderable {
   public:
    TrackView(const Track& track) : mTrack(track) {}

    void render() const override { DrawPlane(mTransform.translation, {50, 50}, LIGHTGRAY); }

   private:
    const Track& mTrack;
};

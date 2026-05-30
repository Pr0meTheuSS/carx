#pragma once
#include "raylib.h"

class Renderable {
   public:
    virtual ~Renderable() = default;

    virtual void render() const = 0;

    Transform& transform() { return mTransform; }
    const Transform& transform() const { return mTransform; }

   protected:
    Transform mTransform;
};
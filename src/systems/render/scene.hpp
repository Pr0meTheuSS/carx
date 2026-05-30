#pragma once
#include <memory>
#include <vector>

#include "renderable.hpp"

class Scene {
   public:
    bool addChild(std::shared_ptr<const Renderable> child);
    std::vector<std::shared_ptr<Renderable>> getChildren() const;

    void render() const; /*{
        for (const auto& child : mChildren) {
            child->render();
        }
    }*/

   private:
    std::vector<std::shared_ptr<const Renderable>> mChildren;
};

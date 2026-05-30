#include "scene.hpp"

bool Scene::addChild(std::shared_ptr<const Renderable> child) {
    mChildren.push_back(child);
    return true;
}

void Scene::render() const {
    for (auto& child : mChildren) {
        child->render();
    }
}

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef Success
#undef Success
#endif
#ifdef None
#undef None
#endif

#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/RenderableManager.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/SwapChain.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <utils/EntityManager.h>

using namespace filament;
using namespace utils;

static const float VERTS[] = {0.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f};

static const uint16_t IND[] = {0, 1, 2};

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* win = SDL_CreateWindow("Filament", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN);

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);

    if (!SDL_GetWindowWMInfo(win, &info)) {
        return -1;
    }

    // 🔥 Filament
    Engine* engine = Engine::create(Engine::Backend::OPENGL);

    SwapChain* swapChain = engine->createSwapChain((void*)info.info.x11.window);

    Renderer* renderer = engine->createRenderer();
    Scene* scene = engine->createScene();
    View* view = engine->createView();

    view->setScene(scene);

    // camera
    Entity cam = EntityManager::get().create();
    Camera* camera = engine->createCamera(cam);
    view->setCamera(camera);

    camera->setProjection(45.0, 1280.0f / 720.0f, 0.1, 100.0);
    camera->lookAt({0, 0, 3}, {0, 0, 0}, {0, 1, 0});

    // geometry
    VertexBuffer* vb =
        VertexBuffer::Builder().vertexCount(3).bufferCount(1).attribute(VertexAttribute::POSITION, 0, VertexBuffer::AttributeType::FLOAT3).build(*engine);

    vb->setBufferAt(*engine, 0, VertexBuffer::BufferDescriptor(VERTS, sizeof(VERTS), nullptr));

    IndexBuffer* ib = IndexBuffer::Builder().indexCount(3).bufferType(IndexBuffer::IndexType::USHORT).build(*engine);

    ib->setBuffer(*engine, IndexBuffer::BufferDescriptor(IND, sizeof(IND), nullptr));

    // entity
    Entity tri = EntityManager::get().create();

    RenderableManager::Builder(1)
        .geometry(0, RenderableManager::PrimitiveType::TRIANGLES, vb, ib, 0, 3)
        .boundingBox({{-1, -1, -1}, {1, 1, 1}})
        .culling(false)
        .build(*engine, tri);

    scene->addEntity(tri);

    // loop
    while (!SDL_QuitRequested()) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) goto exit;
        }

        if (renderer->beginFrame(swapChain)) {
            renderer->render(view);
            renderer->endFrame();
        }
    }

exit:
    Engine::destroy(engine);
    SDL_DestroyWindow(win);
    SDL_Quit();
}
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <btBulletDynamicsCommon.h>
#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "rlgl.h"

// // Для рендера: карта высот (должна быть степенью двойки для удобства)
// static const int TERRAIN_MAP_RES = 128;
struct TrailPoint {
    Vector3 pos;
};

std::vector<TrailPoint> trails[4];
static const int MAX_TRAIL_POINTS = 500;
// =========================
// WORLD SETTINGS (ВАЖНО)
// =========================
static const float WORLD_SCALE = 1.0f;
static const float HEIGHT_SCALE = 1.2f;
static const int TERRAIN_WIDTH = 256;
static const int TERRAIN_LENGTH = 256;

// =========================
// BULLET GLOBALS
// =========================
btDynamicsWorld* world;
btRigidBody* chassisBody;
btRaycastVehicle* vehicle;
btVehicleRaycaster* raycaster;

// =========================
// HEIGHT FUNCTION
// =========================
float GetTerrainHeight(float x, float z) {
    return 0;
    float h = 1.2f * sinf(0.08f * x) + 1.2f * cosf(0.06f * z) + 0.4f * sinf(0.03f * (x + z));

    return h;
}

// =========================
// TERRAIN STORAGE
// =========================
std::vector<float> heightData;

// Возвращает {minH, maxH}
std::pair<float, float> GetTerrainHeightRange(float minX, float maxX, float minZ, float maxZ, int stepsPerAxis = 1) {
    float minH = 99999.0f;
    float maxH = -99999.0f;

    float stepX = (maxX - minX) / stepsPerAxis;
    float stepZ = (maxZ - minZ) / stepsPerAxis;

    // Проходим по сетке внутри заданных границ
    for (int i = 0; i <= stepsPerAxis; ++i) {
        float x = minX + i * stepX;

        for (int j = 0; j <= stepsPerAxis; ++j) {
            float z = minZ + j * stepZ;

            float h = GetTerrainHeight(x, z);

            if (h < minH) minH = h;
            if (h > maxH) maxH = h;
        }
    }

    return {minH, maxH};
}
void DrawTrails() {
    for (int i = 0; i < 4; i++) {
        for (size_t j = 1; j < trails[i].size(); j++) {
            Vector3 a = trails[i][j - 1].pos;
            Vector3 b = trails[i][j].pos;

            DrawLine3D(a, b, RED);
        }
    }
}
// Конвертирует Image (grayscale heightmap) в vector<float> для Bullet
std::vector<float> ImageToHeightData(const Image& img, float maxHeight) {
    std::vector<float> heights;
    // Резервируем память: ширина * высота (это количество вершин)
    heights.resize(img.width * img.height);

    const unsigned char* pixels = static_cast<const unsigned char*>(img.data);

    for (int y = 0; y < img.height; ++y) {     // y здесь соответствует оси Z в мире (глубина)
        for (int x = 0; x < img.width; ++x) {  // x здесь соответствует оси X в мире

            // Вычисляем индекс пикселя в линейном массиве
            // Raylib хранит как [row * width + col]
            int pixelIndex = x * img.height + y;

            // Берем яркость.
            // Если картинка RGB, берем R (или среднее R+G+B).
            // Если Grayscale (как мы делали раньше), R, G, B одинаковы.
            unsigned char val = pixels[pixelIndex * 4];  // Умножаем на 4, т.к. формат обычно UNCOMPRESSED_R8G8B8A8

            // Конвертируем 0..255 в 0..maxHeight
            float h = (static_cast<float>(val) / 255.0f) * maxHeight;

            // Сохраняем в массив.
            // ВАЖНО: Порядок должен совпадать с тем, как ты создаешь shape в Bullet.
            // Обычно это: index = z * width + x.
            // В цикле выше 'y' играет роль 'z' (глубины).
            heights[y * img.width + x] = h;
        }
    }

    return heights;
}

// =========================
// RENDER TERRAIN VIA HEIGHTMAP (GenMeshHeightmap)
// =========================
Model GenerateTerrainModel() {
    // Размер мира в мировых координатах (как в физике)
    size_t worldWidth = TERRAIN_WIDTH * WORLD_SCALE;
    size_t worldLength = TERRAIN_LENGTH * WORLD_SCALE;

    Image image = GenImageColor(worldWidth, worldLength, WHITE);

    Color* pixels = static_cast<Color*>(image.data);
    float minH = 0.0f;  // GetTerrainHeightRange(0, worldWidth, 0, worldLength);
    float maxH = 1.0f;

    for (int y = 0; y < worldWidth; ++y) {
        for (int x = 0; x < worldLength; ++x) {
            float h = heightData[x * worldWidth + y];
            // 1. Нормализуем высоту в диапазон [0, 1]
            // Используем глобальные minH и maxH, которые мы посчитали заранее
            float range = maxH - minH;
            if (range <= 0) range = 1.0f;

            float t = (h - minH) / range;
            t = std::clamp(t, 0.0f, 1.0f);

            // 2. Превращаем в байт 0..255
            unsigned char val = static_cast<unsigned char>(h * 255.0f);

            // 3. Создаем цвет.
            // ВАЖНО: Для GRAYSCALE все три канала (R, G, B) должны быть одинаковыми.
            // Тогда формула внутри SetPixelColor вернет ровно val.
            Color c = {val, val, val, val};

            // 4. Записываем
            SetPixelColor(&pixels[y + x * worldLength], c, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        }
    }

    // Image image = LoadImage("heightmap.png");
    Texture2D texture = LoadTextureFromImage(image);

    // Масштабирование меша:
    // X и Z — размеры мира, Y — высота (подбираем, чтобы выглядело естественно)
    Mesh mesh = GenMeshHeightmap(image, (Vector3){worldWidth, 3.0f, worldLength});

    Model model = LoadModelFromMesh(mesh);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

    UnloadImage(image);  // данные уже в VRAM как текстура

    return model;
}
// =========================
// TERRAIN BUILD
// =========================
void CreateTerrain(btDynamicsWorld* world) {
    heightData.reserve(TERRAIN_LENGTH * TERRAIN_WIDTH);
    Image image = LoadImage("rally_track_heightmap.png");
    heightData = ImageToHeightData(image, 0.3f);

    // 1. Сначала вычисляем высоты
    // PrecomputeHeightfield();

    // 2. Находим мин/макс высоту для нормализации (Bullet любит, когда данные в float)
    // float minH = 99999.0f;
    // float maxH = -99999.0f;
    // for (float h : heightData) {
    //     if (h < minH) minH = h;
    //     if (h > maxH) maxH = h;
    // }

    // Если рельеф плоский, добавим немного высоты, чтобы shape не сломался
    // if (maxH - minH < 0.001f) maxH = minH + 0.1f;

    // 3. Создаем форму terrain
    // Параметры:
    // - width, length: кол-во вершин (не клеток!)
    // - data: указатель на массив float
    // - minHeight, maxHeight: диапазон высот в метрах
    // - scaleX, scaleZ: размер одной клетки в метрах (чтобы покрыть весь размер terrain)
    // - upAxis: 1 = Y (вверх)

    // float cellSizeX = TERRAIN_SIZE_X / (TERRAIN_WIDTH - 1);
    // float cellSizeZ = TERRAIN_SIZE_Z / (TERRAIN_LENGTH - 1);

    btHeightfieldTerrainShape* terrainShape = new btHeightfieldTerrainShape(TERRAIN_WIDTH,      // width (кол-во вершин по X)
                                                                            TERRAIN_LENGTH,     // length (кол-во вершин по Z)
                                                                            heightData.data(),  // указатель на массив float
                                                                            0,                  // minHeight
                                                                            0,                  // maxHeight
                                                                            1,                  // scaleX (размер одной клетки по X в метрах)
                                                                            1,                  // scaleZ (размер одной клетки по Z в метрах)
                                                                            PHY_FLOAT,          // upAxis = 1 (ось Y вверх)
                                                                            true                // useFloatData = true (мы передаём float)
    );
    terrainShape->setUseDiamondSubdivision(true);  // Опционально: улучшает качество коллизий на краях

    // 4. Создаем статический объект (масса 0)
    btTransform groundTransform;
    groundTransform.setIdentity();
    // Сдвигаем центр, если хочешь, чтобы (0,0,0) было в центре карты
    // groundTransform.setOrigin(btVector3(-TERRAIN_SIZE_X / 2, 0, -TERRAIN_SIZE_Z / 2));

    btDefaultMotionState* ms = new btDefaultMotionState(groundTransform);

    btRigidBody::btRigidBodyConstructionInfo ci(0.0f, ms, terrainShape, btVector3(0, 0, 0));
    ci.m_restitution = 0.3f;  // Немного упругости
    ci.m_friction = 0.8f;     // Трение для колес

    btRigidBody* groundBody = new btRigidBody(ci);
    world->addRigidBody(groundBody);
}

// =========================
// PHYSICS INIT
// =========================
void InitPhysics() {
    auto* config = new btDefaultCollisionConfiguration();
    auto* dispatcher = new btCollisionDispatcher(config);
    auto* broadphase = new btDbvtBroadphase();
    auto* solver = new btSequentialImpulseConstraintSolver();

    world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, config);
    world->setGravity(btVector3(0, -9.81f, 0));

    CreateTerrain(world);

    // --- chassis ---
    auto* chassisShape = new btBoxShape(btVector3(1, 0.5, 2));

    btTransform start;
    start.setIdentity();
    start.setOrigin(btVector3(3, 5, 3));

    btScalar mass = 1500;
    btVector3 inertia(0, 0, 0);
    chassisShape->calculateLocalInertia(mass, inertia);

    btDefaultMotionState* ms = new btDefaultMotionState(start);

    btRigidBody::btRigidBodyConstructionInfo ci(mass, ms, chassisShape, inertia);
    chassisBody = new btRigidBody(ci);
    chassisBody->setActivationState(DISABLE_DEACTIVATION);
    btTransform comOffset;
    comOffset.setIdentity();
    comOffset.setOrigin(btVector3(0, 0.0f, 0.0f));

    chassisBody->setCenterOfMassTransform(chassisBody->getWorldTransform() * comOffset);
    world->addRigidBody(chassisBody);

    // --- vehicle tuning (СТАБИЛЬНОСТЬ) ---
    btRaycastVehicle::btVehicleTuning tuning;
    tuning.m_suspensionStiffness = 35.0f;
    tuning.m_suspensionDamping = 0.8f;
    tuning.m_suspensionCompression = 4.5f;
    tuning.m_frictionSlip = 0.9f;

    raycaster = new btDefaultVehicleRaycaster(world);
    vehicle = new btRaycastVehicle(tuning, chassisBody, raycaster);

    world->addVehicle(vehicle);
    vehicle->setCoordinateSystem(0, 1, 2);

    btVector3 points[4] = {btVector3(-1, 0, 2), btVector3(1, 0, 2), btVector3(-1, 0, -2), btVector3(1, 0, -2)};

    for (int i = 0; i < 4; i++) {
        vehicle->addWheel(points[i], btVector3(0, -1, 0), btVector3(-1, 0, 0), 0.2f, 0.7f, tuning, i < 2);
    }
    for (int i = 0; i < 4; i++) {
        auto& w = vehicle->getWheelInfo(i);

        if (i < 2) {
            w.m_frictionSlip = 0.9f;
            w.m_rollInfluence = 0.1f;  // перед
        } else {
            w.m_rollInfluence = 0.15f;  // зад
        }
    }
}

// =========================
// INPUT
// =========================
void UpdateInput() {
    float engine = 0;
    float brake = 0;
    float steer = 0;

    if (IsKeyDown(KEY_W)) engine = 2500;
    if (IsKeyDown(KEY_S)) engine = -2500;

    if (IsKeyDown(KEY_A)) steer = 0.4f;
    if (IsKeyDown(KEY_D)) steer = -0.4f;

    btVector3 vel = chassisBody->getLinearVelocity();
    float speed = vel.length();

    // float maxSteer = 0.4f * (1.0f - (speed / 30.0f, 0.0f, 0.7f));

    static float currentSteer = 0.0f;
    float targetSteer = steer;  // Твой расчётный steer от клавиш

    // Плавное изменение (инерция руля)
    // Чем меньше коэффициент (0.1), тем тяжелее руль
    float steerRate = 0.15f;
    currentSteer += (targetSteer - currentSteer) * steerRate;

    vehicle->setSteeringValue(currentSteer, 0);
    vehicle->setSteeringValue(currentSteer, 1);

    vehicle->applyEngineForce(engine, 2);
    vehicle->applyEngineForce(engine, 3);

    if (IsKeyDown(KEY_SPACE)) {
        // ручник только зад
        vehicle->setBrake(120, 2);
        vehicle->setBrake(120, 3);
    } else {
        for (int i = 0; i < 4; i++) vehicle->setBrake(0, i);
    }
    float speedFactor = 1.0f;

    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_D)) speedFactor = 0.7f;

    vehicle->applyEngineForce(engine * speedFactor, 2);
    vehicle->applyEngineForce(engine * speedFactor, 3);
}

// =========================
// HELPERS
// =========================
Vector3 BtToRl(const btVector3& v) {
    return {v.x(), v.y(), v.z()};
}

Vector3 BtForward(const btTransform& t) {
    btVector3 f = t.getBasis().getColumn(2);
    return {f.x(), f.y(), f.z()};
}

Vector3 BtUp(const btTransform& t) {
    btVector3 u = t.getBasis().getColumn(1);
    return {u.x(), u.y(), u.z()};
}

void DrawWheel(const Vector3& pos, const btMatrix3x3& basis) {
    rlPushMatrix();

    float m[16];

    m[0] = basis[0][0];
    m[4] = basis[0][1];
    m[8] = basis[0][2];
    m[12] = pos.x + 0.15;
    m[1] = basis[1][0];
    m[5] = basis[1][1];
    m[9] = basis[1][2];
    m[13] = pos.y;
    m[2] = basis[2][0];
    m[6] = basis[2][1];
    m[10] = basis[2][2];
    m[14] = pos.z;
    m[3] = 0;
    m[7] = 0;
    m[11] = 0;
    m[15] = 1;

    rlMultMatrixf(m);

    rlRotatef(90.0f, 0, 0, 1);

    DrawCylinder({0, 0, 0}, 0.5f, 0.5f, 0.3f, 16, DARKGRAY);
    DrawCylinderWires({0, 0, 0}, 0.5f, 0.5f, 0.3f, 16, BLACK);

    rlPopMatrix();
}
// =========================
// MAIN
// =========================
int main() {
    InitWindow(1280, 720, "stable vehicle terrain");

    Camera3D cam = {0};
    cam.fovy = 60;
    cam.up = {0, 1, 0};

    InitPhysics();
    SetTargetFPS(60);
    Model chassisModel = LoadModel("assets/subaru/subaru.glb");
    Model terrainModel = GenerateTerrainModel();

    while (!WindowShouldClose()) {
        world->stepSimulation(1.f / 60.f);
        UpdateInput();

        btTransform t;
        chassisBody->getMotionState()->getWorldTransform(t);

        Vector3 pos = BtToRl(t.getOrigin());
        Vector3 forward = BtForward(t);
        Vector3 up = BtUp(t);

        float dist = 8.0f;
        float height = 3.0f;

        cam.target = pos;
        cam.position = {pos.x - forward.x * dist + up.x * height, pos.y - forward.y * dist + up.y * height, pos.z - forward.z * dist + up.z * height};

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(cam);

        DrawModel(terrainModel, (Vector3){-TERRAIN_LENGTH / 2, -1.1, -TERRAIN_LENGTH / 2}, 1.0f, GRAY);
        DrawTrails();
        DrawGrid(TERRAIN_LENGTH, 1);

        // 2. СТРОИМ МАТРИЦУ ТРАНСФОРМАЦИИ ВРУЧНУЮ
        // Мы берем матрицу вращения из Bullet и применяем её к модели.
        float m[16];
        const btMatrix3x3& b = t.getBasis();

        // Заполняем матрицу 4x4 (row-major для raylib)
        // Row 0
        m[0] = b[0][0];
        m[4] = b[0][1];
        m[8] = b[0][2];
        m[12] = pos.x;
        // Row 1
        m[1] = b[1][0];
        m[5] = b[1][1];
        m[9] = b[1][2];
        m[13] = pos.y;
        // Row 2
        m[2] = b[2][0];
        m[6] = b[2][1];
        m[10] = b[2][2];
        m[14] = pos.z;
        // Row 3
        m[3] = 0;
        m[7] = 0;
        m[11] = 0;
        m[15] = 1;

        // --- НАСТРОЙКА МАСШТАБА ---
        // ВАЖНО: Подбери этот коэффициент так, чтобы модель совпала с красным кубом (DrawVehicle)
        // Если модель слишком большая, ставь < 1.0. Если маленькая - > 1.0.
        // Часто для моделей из Blender в метрах подходит 1.0 или 0.5
        float modelScale = 2.5f;

        // 3. ОТРИСОВКА
        rlPushMatrix();
        rlMultMatrixf(m);  // Применяем всю матрицу (позиция + поворот + масштаб) сразу
        // Но лучше исправить это в Blender при экспорте!
        rlRotatef(180.0f, 0, 1, 0);
        DrawModel(chassisModel, (Vector3){0, 0, 0}, modelScale, WHITE);

        rlPopMatrix();

        for (int i = 0; i < vehicle->getNumWheels(); i++) {
            vehicle->updateWheelTransform(i, true);

            auto& wl = vehicle->getWheelInfo(0);
            auto& wr = vehicle->getWheelInfo(1);

            float travelL = wl.m_raycastInfo.m_suspensionLength;
            float travelR = wr.m_raycastInfo.m_suspensionLength;

            float antiRoll = (travelL - travelR) * 8000.0f;

            if (wl.m_raycastInfo.m_isInContact)
                chassisBody->applyForce(btVector3(0, -antiRoll, 0), wl.m_raycastInfo.m_contactPointWS - chassisBody->getCenterOfMassPosition());

            if (wr.m_raycastInfo.m_isInContact)
                chassisBody->applyForce(btVector3(0, antiRoll, 0), wr.m_raycastInfo.m_contactPointWS - chassisBody->getCenterOfMassPosition());

            auto tr = vehicle->getWheelInfo(i).m_worldTransform;
            btVector3 p = tr.getOrigin();

            Vector3 pos = BtToRl(p);

            trails[i].push_back({pos});

            if (trails[i].size() > MAX_TRAIL_POINTS) trails[i].erase(trails[i].begin());

            const btMatrix3x3& b = vehicle->getWheelInfo(i).m_worldTransform.getBasis();

            DrawWheel(BtToRl(tr.getOrigin()), b);
        }

        EndMode3D();
        btVector3 vel = chassisBody->getLinearVelocity();
        float speedKmh = vel.length() * 3.6f;

        DrawText("WASD + SPACE", 20, 20, 20, DARKGRAY);
        DrawText(TextFormat("SPEED  %.1f km/h", speedKmh), 40, 100, 24, BLACK);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

// // #include <raylib.h>

// // #include <vector>

// // #include "utilities/spline2d_generator.h"

// // int main() {
// //     const int W = 1200;
// //     const int H = 800;

// //     InitWindow(W, H, "Spline2D debug");

// //     std::vector<Spline2D::Corner> input = {
// //         Spline2D::Corner(2, Spline2D::Direction::RIGHT),
// //         Spline2D::Corner(2, Spline2D::Direction::RIGHT),
// //         Spline2D::Corner(2, Spline2D::Direction::RIGHT),
// //         Spline2D::Corner(2, Spline2D::Direction::LEFT),
// //     };

// //     auto pts = Spline2D::generateSpline2D(input, 150.0);

// //     SetTargetFPS(60);

// //     while (!WindowShouldClose()) {
// //         BeginDrawing();
// //         ClearBackground(BLACK);

// //         // оси
// //         DrawLine(W / 2, 0, W / 2, H, DARKGRAY);
// //         DrawLine(0, H / 2, W, H / 2, DARKGRAY);

// //         for (size_t i = 1; i < pts.size(); i++) {
// //             DrawLine(W / 2 + pts[i - 1].x, H / 2 + pts[i - 1].y, W / 2 + pts[i].x, H / 2 + pts[i].y, GREEN);

// //             DrawCircle(W / 2 + pts[i].x, H / 2 + pts[i].y, 4, RED);
// //         }

// //         EndDrawing();
// //     }

// //     CloseWindow();
// // }
// // #include "raylib.h"

// // //------------------------------------------------------------------------------------
// // // Program main entry point
// // //------------------------------------------------------------------------------------
// // int main(void) {
// //     // Initialization
// //     //--------------------------------------------------------------------------------------
// //     const int screenWidth = 800;
// //     const int screenHeight = 450;

// //     InitWindow(screenWidth, screenHeight, "raylib [models] example - heightmap rendering");

// //     // Define our custom camera to look into our 3d world
// //     Camera camera = {0};
// //     camera.position = (Vector3){18.0f, 21.0f, 18.0f};  // Camera position
// //     camera.target = (Vector3){0.0f, 0.0f, 0.0f};       // Camera looking at point
// //     camera.up = (Vector3){0.0f, 1.0f, 0.0f};           // Camera up vector (rotation towards target)
// //     camera.fovy = 60.0f;                               // Camera field-of-view Y
// //     camera.projection = CAMERA_PERSPECTIVE;            // Camera projection type

// //     Image image = LoadImage("heightmap.png");         // Load heightmap image (RAM)
// //     Texture2D texture = LoadTextureFromImage(image);  // Convert image to texture (VRAM)

// //     Mesh mesh = GenMeshHeightmap(image, (Vector3){16, 16, 16});  // Generate heightmap mesh (RAM and VRAM)
// //     Model model = LoadModelFromMesh(mesh);                       // Load model from generated mesh

// //     model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;  // Set map diffuse texture
// //     Vector3 mapPosition = {0.0f, 0.0f, 0.0f};                         // Define model position

// //     UnloadImage(image);  // Unload heightmap image from RAM, already uploaded to VRAM

// //     SetTargetFPS(60);  // Set our game to run at 60 frames-per-second
// //     //--------------------------------------------------------------------------------------

// //     // Main game loop
// //     while (!WindowShouldClose())  // Detect window close button or ESC key
// //     {
// //         // Update
// //         //----------------------------------------------------------------------------------
// //         UpdateCamera(&camera, CAMERA_ORBITAL);
// //         //----------------------------------------------------------------------------------

// //         // Draw
// //         //----------------------------------------------------------------------------------
// //         BeginDrawing();

// //         ClearBackground(RAYWHITE);

// //         BeginMode3D(camera);

// //         DrawModel(model, mapPosition, 1.0f, RED);

// //         DrawGrid(20, 1.0f);

// //         EndMode3D();

// //         DrawTexture(texture, screenWidth - texture.width - 20, 20, WHITE);
// //         DrawRectangleLines(screenWidth - texture.width - 20, 20, texture.width, texture.height, GREEN);

// //         DrawFPS(10, 10);

// //         EndDrawing();
// //         //----------------------------------------------------------------------------------
// //     }

// //     // De-Initialization
// //     //--------------------------------------------------------------------------------------
// //     UnloadTexture(texture);  // Unload texture
// //     UnloadModel(model);      // Unload model

// //     CloseWindow();  // Close window and OpenGL context
// //     //--------------------------------------------------------------------------------------

// //     return 0;
// // }

// // #include <stdlib.h>
// // #include <time.h>

// // #include "raylib.h"

// // int main(void) {
// //     const int screenWidth = 800;
// //     const int screenHeight = 450;

// //     InitWindow(screenWidth, screenHeight, "raylib - heightmap from noise");

// //     Camera camera = {0};
// //     camera.position = (Vector3){18.0f, 21.0f, 18.0f};
// //     camera.target = (Vector3){0.0f, 0.0f, 0.0f};
// //     camera.up = (Vector3){0.0f, 1.0f, 0.0f};
// //     camera.fovy = 60.0f;
// //     camera.projection = CAMERA_PERSPECTIVE;

// //     // Параметры карты высот
// //     const int mapSize = 128;  // должно быть степенью двойки для некоторых генераторов
// //     const float scale = 16.0f;

// //     srand((unsigned)time(NULL));

// //     // Создаём изображение-карту высот
// //     Image image = GenImageColor(mapSize, mapSize, WHITE);

// //     for (int y = 0; y < mapSize; ++y) {
// //         for (int x = 0; x < mapSize; ++x) {
// //             // Генерируем шум: случайное значение 0..255
// //             unsigned char val = (unsigned char)(rand() % 256);
// //             Color c = (Color){val, val, val, 255};
// //             unsigned char* pixelsData = static_cast<unsigned char*>(image.data);
// //             SetPixelColor(&pixelsData[x * mapSize + y], c, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
// //         }
// //     }

// //     Texture2D texture = LoadTextureFromImage(image);
// //     Mesh mesh = GenMeshHeightmap(image, (Vector3){scale, 0.1f, scale});
// //     Model model = LoadModelFromMesh(mesh);

// //     model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
// //     Vector3 mapPosition = {0.0f, 0.0f, 0.0f};

// //     UnloadImage(image);  // уже в VRAM как текстура

// //     SetTargetFPS(60);

// //     while (!WindowShouldClose()) {
// //         UpdateCamera(&camera, CAMERA_ORBITAL);

// //         BeginDrawing();
// //         ClearBackground(RAYWHITE);
// //         BeginMode3D(camera);
// //         DrawModel(model, mapPosition, 1.0f, RED);
// //         DrawGrid(20, 1.0f);
// //         EndMode3D();

// //         // Миниатюра карты (опционально)
// //         DrawTexture(texture, screenWidth - texture.width - 20, 20, WHITE);
// //         DrawRectangleLines(screenWidth - texture.width - 20, 20, texture.width, texture.height, GREEN);

// //         DrawFPS(10, 10);
// //         EndDrawing();
// //     }

// //     UnloadTexture(texture);
// //     UnloadModel(model);
// //     CloseWindow();

// //     return 0;
// // }

// /*******************************************************************************************
//  *
//  *   raylib [models] example - mesh generation
//  *
//  *   Example complexity rating: [★★☆☆] 2/4
//  *
//  *   Example originally created with raylib 1.8, last time updated with raylib 4.0
//  *
//  *   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
//  *   BSD-like license that allows static linking with closed source software
//  *
//  *   Copyright (c) 2017-2025 Ramon Santamaria (@raysan5)
//  *
//  ********************************************************************************************/

// // #include <cmath>

// // #include "raylib.h"
// // #define NUM_MODELS 9  // Parametric 3d shapes to generate

// // //------------------------------------------------------------------------------------
// // // Module Functions Declaration
// // //------------------------------------------------------------------------------------
// // static Mesh GenMeshCustom(void);  // Generate a simple triangle mesh from code

// // //------------------------------------------------------------------------------------
// // // Program main entry point
// // //------------------------------------------------------------------------------------
// // int main(void) {
// //     // Initialization
// //     //--------------------------------------------------------------------------------------
// //     const int screenWidth = 800;
// //     const int screenHeight = 450;

// //     InitWindow(screenWidth, screenHeight, "raylib [models] example - mesh generation");

// //     // We generate a checked image for texturing
// //     Image checked = GenImageChecked(2, 2, 1, 1, RED, GREEN);
// //     Texture2D texture = LoadTextureFromImage(checked);
// //     UnloadImage(checked);

// //     Model models[NUM_MODELS] = {0};

// //     models[0] = LoadModelFromMesh(GenMeshPlane(2, 2, 4, 3));
// //     models[1] = LoadModelFromMesh(GenMeshCube(2.0f, 1.0f, 2.0f));
// //     models[2] = LoadModelFromMesh(GenMeshSphere(2, 32, 32));
// //     models[3] = LoadModelFromMesh(GenMeshHemiSphere(2, 16, 16));
// //     models[4] = LoadModelFromMesh(GenMeshCylinder(1, 2, 16));
// //     models[5] = LoadModelFromMesh(GenMeshTorus(0.25f, 4.0f, 16, 32));
// //     models[6] = LoadModelFromMesh(GenMeshKnot(1.0f, 2.0f, 16, 128));
// //     models[7] = LoadModelFromMesh(GenMeshPoly(5, 2.0f));
// //     models[8] = LoadModelFromMesh(GenMeshCustom());

// //     // NOTE: Generated meshes could be exported using ExportMesh()

// //     // Set checked texture as default diffuse component for all models material
// //     for (int i = 0; i < NUM_MODELS; i++) models[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

// //     // Define the camera to look into our 3d world
// //     Camera camera = {{5.0f, 5.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f, 0};

// //     // Model drawing position
// //     Vector3 position = {0.0f, 0.0f, 0.0f};

// //     int currentModel = 0;

// //     SetTargetFPS(60);  // Set our game to run at 60 frames-per-second
// //     //--------------------------------------------------------------------------------------

// //     // Main game loop
// //     while (!WindowShouldClose())  // Detect window close button or ESC key
// //     {
// //         // Update
// //         //----------------------------------------------------------------------------------
// //         UpdateCamera(&camera, CAMERA_ORBITAL);

// //         if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
// //             currentModel = (currentModel + 1) % NUM_MODELS;  // Cycle between the textures
// //         }

// //         if (IsKeyPressed(KEY_RIGHT)) {
// //             currentModel++;
// //             if (currentModel >= NUM_MODELS) currentModel = 0;
// //         } else if (IsKeyPressed(KEY_LEFT)) {
// //             currentModel--;
// //             if (currentModel < 0) currentModel = NUM_MODELS - 1;
// //         }
// //         //----------------------------------------------------------------------------------

// //         // Draw
// //         //----------------------------------------------------------------------------------
// //         BeginDrawing();

// //         ClearBackground(RAYWHITE);

// //         BeginMode3D(camera);

// //         DrawModel(models[currentModel], position, 1.0f, WHITE);
// //         DrawGrid(10, 1.0);

// //         EndMode3D();

// //         DrawRectangle(30, 400, 310, 30, Fade(SKYBLUE, 0.5f));
// //         DrawRectangleLines(30, 400, 310, 30, Fade(DARKBLUE, 0.5f));
// //         DrawText("MOUSE LEFT BUTTON to CYCLE PROCEDURAL MODELS", 40, 410, 10, BLUE);

// //         switch (currentModel) {
// //             case 0:
// //                 DrawText("PLANE", 680, 10, 20, DARKBLUE);
// //                 break;
// //             case 1:
// //                 DrawText("CUBE", 680, 10, 20, DARKBLUE);
// //                 break;
// //             case 2:
// //                 DrawText("SPHERE", 680, 10, 20, DARKBLUE);
// //                 break;
// //             case 3:
// //                 DrawText("HEMISPHERE", 640, 10, 20, DARKBLUE);
// //                 break;
// //             case 4:
// //                 DrawText("CYLINDER", 680, 10, 20, DARKBLUE);
// //                 break;
// //             case 5:
// //                 DrawText("TORUS", 680, 10, 20, DARKBLUE);
// //                 break;
// //             case 6:
// //                 DrawText("KNOT", 680, 10, 20, DARKBLUE);
// //                 break;
// //             case 7:
// //                 DrawText("POLY", 680, 10, 20, DARKBLUE);
// //                 break;
// //             case 8:
// //                 DrawText("Custom (triangle)", 580, 10, 20, DARKBLUE);
// //                 break;
// //             default:
// //                 break;
// //         }

// //         EndDrawing();
// //         //----------------------------------------------------------------------------------
// //     }

// //     // De-Initialization
// //     //--------------------------------------------------------------------------------------
// //     UnloadTexture(texture);  // Unload texture

// //     // Unload models data (GPU VRAM)
// //     for (int i = 0; i < NUM_MODELS; i++) UnloadModel(models[i]);

// //     CloseWindow();  // Close window and OpenGL context
// //     //--------------------------------------------------------------------------------------

// //     return 0;
// // }

// // //------------------------------------------------------------------------------------
// // // Module Functions Definition
// // //------------------------------------------------------------------------------------
// // // Generate a simple triangle mesh from code
// // // Функция ручной генерации меша террейна
// // static Mesh GenMeshCustom() {
// //     Mesh mesh = {0};
// //     int width = 20;
// //     int height = 20;
// //     float cellSize = 1;
// //     float maxHeight = 2;
// //     // Количество вершин: каждая ячейка сетки - это 2 треугольника = 6 вершин,
// //     // но вершины мы будем переиспользовать. Для простой сетки нам нужно width * height вершин.
// //     mesh.vertexCount = width * height;

// //     // Количество треугольников: (width-1)*(height-1) ячеек * 2 треугольника в ячейке
// //     int trianglesCount = (width - 1) * (height - 1) * 2;
// //     mesh.triangleCount = trianglesCount;

// //     // Выделяем память
// //     mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
// //     mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
// //     mesh.normals = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
// //     mesh.indices = (unsigned short*)MemAlloc(trianglesCount * 3 * sizeof(unsigned short));

// //     if (!mesh.vertices || !mesh.texcoords || !mesh.normals || !mesh.indices) {
// //         TraceLog(LOG_ERROR, "Не удалось выделить память для меша!");
// //         return mesh;
// //     }

// //     // 1. Заполняем вершины, UV и случайные высоты
// //     for (int z = 0; z < height; z++) {
// //         for (int x = 0; x < width; x++) {
// //             int index = z * width + x;

// //             // Позиция вершины
// //             float posX = (float)x * cellSize;
// //             float posZ = (float)z * cellSize;

// //             // Случайная высота (от 0 до maxHeight)
// //             float randomH = (float)GetRandomValue(0, 100) / 100.0f * maxHeight;
// //             float posY = randomH;

// //             mesh.vertices[index * 3 + 0] = posX;
// //             mesh.vertices[index * 3 + 1] = posY;
// //             mesh.vertices[index * 3 + 2] = posZ;

// //             // UV координаты (от 0 до 1 по всей текстуре)
// //             mesh.texcoords[index * 2 + 0] = (float)x / (width - 1);
// //             mesh.texcoords[index * 2 + 1] = (float)z / (height - 1);
// //         }
// //     }

// //     // 2. Заполняем индексы (строим треугольники)
// //     int idx = 0;
// //     for (int z = 0; z < height - 1; z++) {
// //         for (int x = 0; x < width - 1; x++) {
// //             int lt = z * width + x;              // Left Top
// //             int rt = z * width + (x + 1);        // Right Top
// //             int lb = (z + 1) * width + x;        // Left Bottom
// //             int rb = (z + 1) * width + (x + 1);  // Right Bottom

// //             // Первый треугольник (LT -> RT -> LB)
// //             mesh.indices[idx++] = (unsigned short)lt;
// //             mesh.indices[idx++] = (unsigned short)rt;
// //             mesh.indices[idx++] = (unsigned short)lb;

// //             // Второй треугольник (RT -> RB -> LB)
// //             mesh.indices[idx++] = (unsigned short)rt;
// //             mesh.indices[idx++] = (unsigned short)rb;
// //             mesh.indices[idx++] = (unsigned short)lb;
// //         }
// //     }

// //     // 3. Считаем нормали вручную
// //     // Нормаль вершины = усреднённая нормаль всех треугольников, в которых участвует эта вершина
// //     // Для простоты в этом примере мы сделаем упрощённый расчёт:
// //     // возьмём нормаль плоскости, образованной соседями (крест-накрест).
// //     for (int i = 0; i < mesh.vertexCount; i++) {
// //         float nx = 0.0f, ny = 0.0f, nz = 0.0f;
// //         int count = 0;

// //         int x = i % width;
// //         int z = i / width;

// //         // Проверяем 4 соседа (верх, низ, лево, право)
// //         int offsets[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

// //         for (int k = 0; k < 4; k++) {
// //             int nxCoord = x + offsets[k][0];
// //             int nzCoord = z + offsets[k][1];

// //             if (nxCoord >= 0 && nxCoord < width && nzCoord >= 0 && nzCoord < height) {
// //                 int neighborIdx = nzCoord * width + nxCoord;

// //                 // Вектор от текущей вершины к соседу
// //                 float dx = mesh.vertices[neighborIdx * 3 + 0] - mesh.vertices[i * 3 + 0];
// //                 float dy = mesh.vertices[neighborIdx * 3 + 1] - mesh.vertices[i * 3 + 1];
// //                 float dz = mesh.vertices[neighborIdx * 3 + 2] - mesh.vertices[i * 3 + 2];

// //                 // Накопляем направление. Это упрощённая аппроксимация нормали поверхности.
// //                 // Более правильно было бы брать векторное произведение пар соседей, но этот метод даёт хороший визуальный результат для шума.
// //                 nx += dx;
// //                 ny += dy;
// //                 nz += dz;
// //                 count++;
// //             }
// //         }

// //         if (count > 0) {
// //             nx /= count;
// //             ny /= count;
// //             nz /= count;
// //         }

// //         // Нормализуем вектор
// //         float len = sqrt(nx * nx + ny * ny + nz * nz);
// //         if (len > 0.0001f) {
// //             mesh.normals[i * 3 + 0] = nx / len;
// //             mesh.normals[i * 3 + 1] = ny / len;
// //             mesh.normals[i * 3 + 2] = nz / len;
// //         } else {
// //             // Если вершина плоская (край или ошибка), ставим вверх
// //             mesh.normals[i * 3 + 0] = 0.0f;
// //             mesh.normals[i * 3 + 1] = 1.0f;
// //             mesh.normals[i * 3 + 2] = 0.0f;
// //         }
// //     }

// //     // Загружаем меш в видеопамять
// //     UploadMesh(&mesh, false);

// //     return mesh;
// // }

// #include <raylib.h>

// #include <algorithm>
// #include <cmath>
// #include <vector>

// int main() {
//     const int W = 256;
//     const int H = 256;

//     Image img = GenImageColor(W, H, BLACK);
//     unsigned char* pixels = static_cast<unsigned char*>(img.data);

//     // Параметры трассы
//     float trackWidth = 40.0f;    // ширина трассы в клетках
//     float centerRadius = 80.0f;  // радиус петли
//     float waveAmp = 2.0f;        // амплитуда волн

//     for (int y = 0; y < H; ++y) {
//         for (int x = 0; x < W; ++x) {
//             float fx = (float)x - (float)W / 2.0f;
//             float fy = (float)y - (float)H / 2.0f;

//             // Спиральная/петлевая трасса (полярная система)
//             float r = sqrt(fx * fx + fy * fy);
//             float theta = atan2(fy, fx);

//             // Делаем «волну» вдоль трассы (вдоль угла)
//             float distFromCenter = r - centerRadius;
//             float isOnTrack = 1.0f - std::clamp(std::abs(distFromCenter) / (trackWidth / 2.0f), 0.0f, 1.0f);

//             // Высота: базовая волна + рельеф трассы
//             float h = waveAmp * sinf(theta * 4.0f) * isOnTrack;
//             h += 0.5f * (1.0f - isOnTrack);  // чуть выше обочины

//             // Нормализуем в 0..1
//             h = std::clamp((h + 2.0f) / 4.0f, 0.0f, 1.0f);

//             unsigned char val = static_cast<unsigned char>(h * 255.0f);
//             Color c = {val, val, val, 255};

//             SetPixelColor(&pixels[y * W + x], c, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
//         }
//     }

//     ExportImage(img, "rally_track_heightmap.png");
//     UnloadImage(img);
//     return 0;
// }
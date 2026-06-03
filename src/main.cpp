#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <btBulletDynamicsCommon.h>
#include <raylib.h>

#include <cmath>
#include <vector>

#include "rlgl.h"

struct TrailPoint {
    Vector3 pos;
};

std::vector<TrailPoint> trails[4];
static const int MAX_TRAIL_POINTS = 500;
// =========================
// WORLD SETTINGS (ВАЖНО)
// =========================
static const float WORLD_SCALE = 2.0f;
static const float HEIGHT_SCALE = 1.2f;
static const int TERRAIN_SIZE = 80;

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
    float h = 1.2f * sinf(0.08f * x) + 1.2f * cosf(0.06f * z) + 0.4f * sinf(0.03f * (x + z));

    return h;
}

// =========================
// TERRAIN STORAGE
// =========================
std::vector<float> heightData;

// =========================
// SAMPLE (same source for physics & render)
// =========================
float SampleHeight(int x, int z) {
    return heightData[x * TERRAIN_SIZE + z];
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
// =========================
// TERRAIN BUILD (NO HEIGHTFIELD BUGS)
// =========================
void CreateTerrain(btDynamicsWorld* world) {
    heightData.resize(TERRAIN_SIZE * TERRAIN_SIZE);

    btCollisionShape* box = new btBoxShape(btVector3(1, 0.1f, 1));

    for (int x = 0; x < TERRAIN_SIZE; x++)
        for (int z = 0; z < TERRAIN_SIZE; z++) {
            float wx = (x - TERRAIN_SIZE / 2);
            float wz = (z - TERRAIN_SIZE / 2);

            float h = GetTerrainHeight(wx, wz);
            heightData[x * TERRAIN_SIZE + z] = h;

            btTransform t;
            t.setIdentity();
            t.setOrigin(btVector3(wx * WORLD_SCALE, h, wz * WORLD_SCALE));

            btDefaultMotionState* ms = new btDefaultMotionState(t);

            btRigidBody::btRigidBodyConstructionInfo ci(0.0f, ms, box);

            btRigidBody* body = new btRigidBody(ci);
            world->addRigidBody(body);
        }
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
    start.setOrigin(btVector3(0, 5, 0));

    btScalar mass = 800;
    btVector3 inertia(0, 0, 0);
    chassisShape->calculateLocalInertia(mass, inertia);

    btDefaultMotionState* ms = new btDefaultMotionState(start);

    btRigidBody::btRigidBodyConstructionInfo ci(mass, ms, chassisShape, inertia);
    chassisBody = new btRigidBody(ci);
    chassisBody->setActivationState(DISABLE_DEACTIVATION);

    world->addRigidBody(chassisBody);

    // --- vehicle tuning (СТАБИЛЬНОСТЬ) ---
    btRaycastVehicle::btVehicleTuning tuning;
    tuning.m_suspensionStiffness = 18.0f;
    tuning.m_suspensionDamping = 3.5f;
    tuning.m_suspensionCompression = 4.5f;
    tuning.m_frictionSlip = 2.2f;

    raycaster = new btDefaultVehicleRaycaster(world);
    vehicle = new btRaycastVehicle(tuning, chassisBody, raycaster);

    world->addVehicle(vehicle);
    vehicle->setCoordinateSystem(0, 1, 2);

    btVector3 points[4] = {btVector3(-1, 0, 2), btVector3(1, 0, 2), btVector3(-1, 0, -2), btVector3(1, 0, -2)};

    for (int i = 0; i < 4; i++) {
        vehicle->addWheel(points[i], btVector3(0, -1, 0), btVector3(-1, 0, 0), 0.6f,
                          0.7f,  // <- чуть увеличено (СТАБИЛЬНЫЙ контакт)
                          tuning, i < 2);
    }
    for (int i = 0; i < 4; i++) {
        auto& w = vehicle->getWheelInfo(i);

        if (i < 2) {
            // передние колёса — контроль
            w.m_frictionSlip = 3.5f;
            w.m_rollInfluence = 0.1f;
        } else {
            // задние — дрифт
            w.m_frictionSlip = 1.6f;
            w.m_rollInfluence = 0.6f;
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

    if (IsKeyDown(KEY_W)) engine = 1500;
    if (IsKeyDown(KEY_S)) engine = -1500;

    if (IsKeyDown(KEY_A)) steer = 0.4f;
    if (IsKeyDown(KEY_D)) steer = -0.4f;

    vehicle->setSteeringValue(steer, 0);
    vehicle->setSteeringValue(steer, 1);

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
// RENDER TERRAIN
// =========================
void DrawTerrain() {
    for (int x = 0; x < TERRAIN_SIZE - 1; x++)
        for (int z = 0; z < TERRAIN_SIZE - 1; z++) {
            float h = SampleHeight(x, z);

            Vector3 pos = {(x - TERRAIN_SIZE / 2) * WORLD_SCALE, h, (z - TERRAIN_SIZE / 2) * WORLD_SCALE};

            DrawCube(pos, WORLD_SCALE, 0.2f, WORLD_SCALE, Color{80, (unsigned char)(120 + h * 15), 80, 255});
        }
}

// =========================
// DRAW CHASSIS
// =========================
void DrawVehicle(const btTransform& t) {
    float m[16];
    const btMatrix3x3& b = t.getBasis();
    const btVector3& o = t.getOrigin();

    m[0] = b[0][0];
    m[4] = b[0][1];
    m[8] = b[0][2];
    m[12] = o.x();
    m[1] = b[1][0];
    m[5] = b[1][1];
    m[9] = b[1][2];
    m[13] = o.y();
    m[2] = b[2][0];
    m[6] = b[2][1];
    m[10] = b[2][2];
    m[14] = o.z();
    m[3] = 0;
    m[7] = 0;
    m[11] = 0;
    m[15] = 1;

    rlPushMatrix();
    rlMultMatrixf(m);
    DrawCube({0, 0, 0}, 2, 1, 4, RED);
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

        DrawTerrain();
        DrawTrails();

        DrawVehicle(t);

        for (int i = 0; i < vehicle->getNumWheels(); i++) {
            vehicle->updateWheelTransform(i, true);

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
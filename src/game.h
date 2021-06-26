#ifndef MAIN_H
#define MAIN_H

#include <sl3dge/sl3dge.h>

struct Renderer;
struct Mesh;

typedef struct CameraMatrices {
    alignas(16) Mat4 proj;
    alignas(16) Mat4 view;
    alignas(16) Mat4 view_inverse;
    alignas(16) Mat4 shadow_mvp;
    alignas(16) Vec3 pos;
    alignas(16) Vec3 light_dir;
} CameraMatrices;

typedef struct GameData {
    CameraMatrices matrices;
    Vec3 position;
    Vec2f spherical_coordinates;
    Vec3 light_pos;
    f32 cos;
} GameData;

typedef void fn_GameStart(GameData *game_data, Renderer *renderer);
fn_GameStart *pfn_GameStart;

typedef void fn_GameLoop(float delta_time, GameData *game_data, Renderer *renderer);
fn_GameLoop *pfn_GameLoop;

#endif // MAIN_H

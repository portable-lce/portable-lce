#pragma once
#include <memory>

#include "java/FloatBuffer.h"
#include "java/IntBuffer.h"

class TilePos;
class Vec3;
class Player;
class Mob;
class FloatBuffer;
class Level;
class LivingEntity;

class Camera {
public:
    static float xPlayerOffs;
    static float yPlayerOffs;
    static float zPlayerOffs;

private:
    //	static IntBuffer *viewport;
    static FloatBuffer* modelview;
    static FloatBuffer* projection;
    //	static FloatBuffer *position;

public:
    static float xa, ya, za, xa2, za2;

    static const float* getModelviewData();
    static const float* getProjectionData();
    static void prepare(std::shared_ptr<Player> player, bool mirror);

    static TilePos* getCameraTilePos(std::shared_ptr<LivingEntity> player,
                                     double alpha);
    static Vec3 getCameraPos(std::shared_ptr<LivingEntity> player,
                             double alpha);
    static int getBlockAt(Level* level, std::shared_ptr<LivingEntity> player,
                          float alpha);
};

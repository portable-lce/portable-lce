#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "java/Random.h"
#include "minecraft/client/model/SkinBox.h"

class Mob;
class ModelPart;
class TexOffs;
class LivingEntity;
class Entity;

class Model {
public:
    float attackTime;
    bool riding;
    std::vector<ModelPart*> cubes;
    bool young;
    std::unordered_map<std::string, TexOffs*> mappedTexOffs;
    int texWidth;
    int texHeight;

    Model();  // 4J added
    virtual void render(std::shared_ptr<Entity> entity, float time, float r,
                        float bob, float yRot, float xRot, float scale,
                        bool usecompiled) {}
    virtual void setupAnim(float time, float r, float bob, float yRot,
                           float xRot, float scale,
                           std::shared_ptr<Entity> entity,
                           unsigned int uiBitmaskOverrideAnim = 0) {}
    virtual void prepareMobModel(std::shared_ptr<LivingEntity> mob, float time,
                                 float r, float a) {}
    virtual ModelPart* getRandomModelPart(Random random) {
        return cubes.at(random.nextInt((int)cubes.size()));
    }
    virtual ModelPart* AddOrRetrievePart(SKIN_BOX* pBox) { return nullptr; }

    void setMapTex(std::string id, int x, int y);
    TexOffs* getMapTex(std::string id);

protected:
    float yHeadOffs;
    float zHeadOffs;
};

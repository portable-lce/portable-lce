
#pragma once
#include "minecraft/client/model/geom/Model.h"

class ModelPart;

class BookModel : public Model {
public:
    ModelPart *leftLid, *rightLid;
    ModelPart *leftPages, *rightPages;
    ModelPart *flipPage1, *flipPage2;
    ModelPart* seam;

    BookModel();
    virtual void render(std::shared_ptr<Entity> entity, float time, float r,
                        float bob, float yRot, float xRot, float scale,
                        bool usecompiled);
    virtual void setupAnim(float time, float r, float bob, float yRot,
                           float xRot, float scale,
                           std::shared_ptr<Entity> entity,
                           unsigned int uiBitmaskOverrideAnim = 0);
};

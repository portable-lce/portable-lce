#include "Lighting.h"

#include <numbers>

#include "java/FloatBuffer.h"
#include "minecraft/world/phys/Vec3.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

FloatBuffer* Lighting::lb = new FloatBuffer(16);

void Lighting::turnOff() {
    RenderPath.StateSetLightingEnable(false);
    RenderPath.StateSetLightEnable(0, false);
    RenderPath.StateSetLightEnable(1, false);
    (void)0;
}

void Lighting::turnOn() {
    RenderPath.StateSetLightingEnable(true);
    RenderPath.StateSetLightEnable(0, true);
    RenderPath.StateSetLightEnable(1, true);
    (void)0;
    (void)0;
    float a = 0.4f;
    float d = 0.6f;
    float s = 0.0f;

    // PLCE: these mirror the legacy RenderHelper LIGHT0/LIGHT1 constants
    // (toward-light directions, model space): primary from above-left-front,
    // fill from below-right-behind.
    Vec3 l(-0.2f, 1.0f, -0.7f);
    l = l.normalize();
    RenderPath.StateSetLightDirection(0, l.x, l.y, l.z);
    RenderPath.StateSetLightColour(0, d, d, d);

    l = Vec3(0.2f, -1.0f, 0.7f);
    l = l.normalize();
    RenderPath.StateSetLightDirection(1, l.x, l.y, l.z);
    RenderPath.StateSetLightColour(1, d, d, d);

    RenderPath.StateSetLightAmbientColour(a, a, a);
}

FloatBuffer* Lighting::getBuffer(double a, double b, double c, double d) {
    return getBuffer((float)a, (float)b, (float)c, (float)d);
}

FloatBuffer* Lighting::getBuffer(float a, float b, float c, float d) {
    lb->clear();
    lb->put(a)->put(b)->put(c)->put(d);
    lb->flip();
    return lb;
}

void Lighting::turnOnGui() {
    RenderPath.MatrixPush();
    RenderPath.MatrixRotate((-30)*(std::numbers::pi_v<float>/180.f), 0, 1, 0);
    RenderPath.MatrixRotate((165)*(std::numbers::pi_v<float>/180.f), 1, 0, 0);
    turnOn();
    RenderPath.MatrixPop();
}

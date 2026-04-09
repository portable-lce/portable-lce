#include "Frustum.h"

#include <string.h>

#include <cmath>
#include <vector>

#include "java/FloatBuffer.h"
#include "minecraft/client/MemoryTracker.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

class FrustumData;

Frustum* Frustum::frustum = new Frustum();

// those are now unused but i still gotta do testing.
Frustum::Frustum() {
    _proj = MemoryTracker::createFloatBuffer(16);
    _modl = MemoryTracker::createFloatBuffer(16);
    _clip = MemoryTracker::createFloatBuffer(16);
}

Frustum::~Frustum() {
    delete _proj;
    delete _modl;
    delete _clip;
}

FrustumData* Frustum::getFrustum() {
    frustum->calculateFrustum();
    return frustum;
}

///////////////////////////////// NORMALIZE PLANE
///\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*
/////
/////	This normalizes a plane (A side) from a given frustum.
/////
///////////////////////////////// NORMALIZE PLANE
///\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*

void Frustum::normalizePlane(float** frustum, int side) {
    float magnitude = (float)sqrt(frustum[side][A] * frustum[side][A] +
                                  frustum[side][B] * frustum[side][B] +
                                  frustum[side][C] * frustum[side][C]);

    // Then we divide the plane's values by it's magnitude.
    // This makes it easier to work with.
    frustum[side][A] /= magnitude;
    frustum[side][B] /= magnitude;
    frustum[side][C] /= magnitude;
    frustum[side][D] /= magnitude;
}

void Frustum::calculateFrustum() {
    // 4jcraft: GL 3.3 core removed GL_MODELVIEW_MATRIX / GL_PROJECTION_MATRIX
    // queries.
    // Camera::prepare() already captures both matrices every frame :)
    // i spent an ungodly amount of time on this simple fix.
    memcpy(proj.data(), PlatformRenderer.MatrixGet(GL_PROJECTION_MATRIX),
           16 * sizeof(float));
    memcpy(modl.data(), PlatformRenderer.MatrixGet(GL_MODELVIEW_MATRIX),
           16 * sizeof(float));

    float* p = proj.data();
    float* m = modl.data();
    float* c = clip.data();

    c[0] = m[0] * p[0] + m[1] * p[4] + m[2] * p[8] + m[3] * p[12];
    c[1] = m[0] * p[1] + m[1] * p[5] + m[2] * p[9] + m[3] * p[13];
    c[2] = m[0] * p[2] + m[1] * p[6] + m[2] * p[10] + m[3] * p[14];
    c[3] = m[0] * p[3] + m[1] * p[7] + m[2] * p[11] + m[3] * p[15];

    c[4] = m[4] * p[0] + m[5] * p[4] + m[6] * p[8] + m[7] * p[12];
    c[5] = m[4] * p[1] + m[5] * p[5] + m[6] * p[9] + m[7] * p[13];
    c[6] = m[4] * p[2] + m[5] * p[6] + m[6] * p[10] + m[7] * p[14];
    c[7] = m[4] * p[3] + m[5] * p[7] + m[6] * p[11] + m[7] * p[15];

    c[8] = m[8] * p[0] + m[9] * p[4] + m[10] * p[8] + m[11] * p[12];
    c[9] = m[8] * p[1] + m[9] * p[5] + m[10] * p[9] + m[11] * p[13];
    c[10] = m[8] * p[2] + m[9] * p[6] + m[10] * p[10] + m[11] * p[14];
    c[11] = m[8] * p[3] + m[9] * p[7] + m[10] * p[11] + m[11] * p[15];

    c[12] = m[12] * p[0] + m[13] * p[4] + m[14] * p[8] + m[15] * p[12];
    c[13] = m[12] * p[1] + m[13] * p[5] + m[14] * p[9] + m[15] * p[13];
    c[14] = m[12] * p[2] + m[13] * p[6] + m[14] * p[10] + m[15] * p[14];
    c[15] = m[12] * p[3] + m[13] * p[7] + m[14] * p[11] + m[15] * p[15];

    // Now we actually want to get the sides of the frustum.  To do this we take
    // the clipping planes we received above and extract the sides from them.

    // This will extract the RIGHT side of the frustum
    m_Frustum[RIGHT][A] = clip[3] - clip[0];
    m_Frustum[RIGHT][B] = clip[7] - clip[4];
    m_Frustum[RIGHT][C] = clip[11] - clip[8];
    m_Frustum[RIGHT][D] = clip[15] - clip[12];

    // Now that we have a normal (A,B,C) and a distance (D) to the plane,
    // we want to normalize that normal and distance.

    // Normalize the RIGHT side
    normalizePlane(m_Frustum, RIGHT);

    // This will extract the LEFT side of the frustum
    m_Frustum[LEFT][A] = clip[3] + clip[0];
    m_Frustum[LEFT][B] = clip[7] + clip[4];
    m_Frustum[LEFT][C] = clip[11] + clip[8];
    m_Frustum[LEFT][D] = clip[15] + clip[12];

    // Normalize the LEFT side
    normalizePlane(m_Frustum, LEFT);

    // This will extract the BOTTOM side of the frustum
    m_Frustum[BOTTOM][A] = clip[3] + clip[1];
    m_Frustum[BOTTOM][B] = clip[7] + clip[5];
    m_Frustum[BOTTOM][C] = clip[11] + clip[9];
    m_Frustum[BOTTOM][D] = clip[15] + clip[13];

    // Normalize the BOTTOM side
    normalizePlane(m_Frustum, BOTTOM);

    // This will extract the TOP side of the frustum
    m_Frustum[TOP][A] = clip[3] - clip[1];
    m_Frustum[TOP][B] = clip[7] - clip[5];
    m_Frustum[TOP][C] = clip[11] - clip[9];
    m_Frustum[TOP][D] = clip[15] - clip[13];

    // Normalize the TOP side
    normalizePlane(m_Frustum, TOP);

    // This will extract the BACK side of the frustum
    m_Frustum[BACK][A] = clip[3] - clip[2];
    m_Frustum[BACK][B] = clip[7] - clip[6];
    m_Frustum[BACK][C] = clip[11] - clip[10];
    m_Frustum[BACK][D] = clip[15] - clip[14];

    // Normalize the BACK side
    normalizePlane(m_Frustum, BACK);

    // This will extract the FRONT side of the frustum
    m_Frustum[FRONT][A] = clip[3] + clip[2];
    m_Frustum[FRONT][B] = clip[7] + clip[6];
    m_Frustum[FRONT][C] = clip[11] + clip[10];
    m_Frustum[FRONT][D] = clip[15] + clip[14];

    // Normalize the FRONT side
    normalizePlane(m_Frustum, FRONT);
}
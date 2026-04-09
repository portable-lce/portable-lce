
// 4J - added for common ctor code
// Do all the default initialisations done in the java class
#include "minecraft/world/level/pathfinder/Node.h"

#include <stdint.h>

#include <string>

#include "minecraft/util/Mth.h"
#include "util/StringHelpers.h"

void Node::_init() {
    heapIdx = -1;

    closed = false;

    cameFrom = nullptr;
}

Node::Node(const int x, const int y, const int z)
    : x(x), y(y), z(z), hash(createHash(x, y, z)) {
    _init();

    // this->x = x;
    // this->y = y;
    // this->z = z;

    // hash = createHash(x, y, z);
}

int Node::createHash(const int x, const int y, const int z) {
    // 4jcraft added cast to higher value to be representable after shift
    return (y & 0xff) | (((int64_t)x & 0x7fff) << 8) |
           (((int64_t)z & 0x7fff) << 24) | ((x < 0) ? 0x0080000000 : 0) |
           ((z < 0) ? 0x0000008000 : 0);
}

float Node::distanceTo(Node* to) {
    float xd = (float)(to->x - x);
    float yd = (float)(to->y - y);
    float zd = (float)(to->z - z);
    return Mth::sqrt(xd * xd + yd * yd + zd * zd);
}

float Node::distanceToSqr(Node* to) {
    float xd = to->x - x;
    float yd = to->y - y;
    float zd = to->z - z;
    return xd * xd + yd * yd + zd * zd;
}

bool Node::equals(Node* o) {
    // 4J Jev, never used anything other than a node.
    // if (dynamic_cast<Node *>((Node *) o) != nullptr)
    //{
    return hash == o->hash && x == o->x && y == o->y && z == o->z;
    //}
    // return false;
}

int Node::hashCode() { return hash; }

bool Node::inOpenSet() { return heapIdx >= 0; }

std::string Node::toString() {
    return toWString<int>(x) + ", " + toWString<int>(y) + ", " +
           toWString<int>(z);
}

#include "Path.h"

#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/level/pathfinder/Node.h"
#include "minecraft/world/phys/Vec3.h"

Path::Path(std::vector<Node*>& src)
    : nodes(src.size()), index(0), length(static_cast<int>(src.size())) {
    for (int i = 0; i < length; i++) nodes[i] = *src[i];
}

void Path::next() { index++; }

bool Path::isDone() { return index >= length; }

Node* Path::last() {
    if (length > 0) {
        return &nodes[length - 1];
    }
    return nullptr;
}

Node* Path::get(int i) { return &nodes[i]; }

int Path::getSize() { return length; }

void Path::setSize(int length) { this->length = length; }

int Path::getIndex() { return index; }

void Path::setIndex(int index) { this->index = index; }

Vec3 Path::getPos(std::shared_ptr<Entity> e, int index) {
    double x = nodes[index].x + (int)(e->bbWidth + 1) * 0.5;
    double y = nodes[index].y;
    double z = nodes[index].z + (int)(e->bbWidth + 1) * 0.5;
    return Vec3(x, y, z);
}

Vec3 Path::currentPos(std::shared_ptr<Entity> e) { return getPos(e, index); }

Vec3 Path::currentPos() {
    return Vec3(nodes[index].x, nodes[index].y, nodes[index].z);
}

bool Path::sameAs(Path* path) {
    if (path == nullptr) return false;
    if (path->nodes.size() != nodes.size()) return false;
    for (size_t i = 0; i < nodes.size(); ++i)
        if (nodes[i].x != path->nodes[i].x || nodes[i].y != path->nodes[i].y ||
            nodes[i].z != path->nodes[i].z)
            return false;
    return true;
}

bool Path::endsIn(Vec3* pos) {
    Node* lastNode = last();
    if (lastNode == nullptr) return false;
    return lastNode->x == (int)pos->x && lastNode->y == (int)pos->y &&
           lastNode->z == (int)pos->z;
}

bool Path::endsInXZ(Vec3* pos) {
    Node* lastNode = last();
    if (lastNode == nullptr) return false;
    return lastNode->x == (int)pos->x && lastNode->z == (int)pos->z;
}

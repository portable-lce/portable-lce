#pragma once

#include <format>
#include <memory>
#include <vector>

#include "minecraft/world/level/pathfinder/Node.h"
#include "minecraft/world/phys/Vec3.h"

class Entity;

class Path {
    friend class PathFinder;

private:
    // Nodes are stored by value. The vector is sized once in the constructor
    // and never resized, so Node* returned by get()/last() remain valid for
    // the lifetime of the Path.
    std::vector<Node> nodes;
    int index;
    int length;

public:
    Path(std::vector<Node*>& nodes);
    ~Path() = default;

    void next();
    bool isDone();
    Node* last();
    Node* get(int i);
    int getSize();
    void setSize(int length);
    int getIndex();
    void setIndex(int index);
    Vec3 getPos(std::shared_ptr<Entity> e, int index);
    Vec3 currentPos(std::shared_ptr<Entity> e);
    Vec3 currentPos();
    bool sameAs(Path* path);
    bool endsIn(Vec3* pos);
    bool endsInXZ(Vec3* pos);
};

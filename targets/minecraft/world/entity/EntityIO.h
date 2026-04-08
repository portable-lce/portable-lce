#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "Entity.h"
#include "minecraft/GameEnums.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "java/Class.h"
#include "java/JavaIntHash.h"

class Level;
class CompoundTag;
struct eINSTANCEOFKeyEq;
struct eINSTANCEOFKeyHash;

typedef Entity* (*entityCreateFn)(Level*);
class EntityIO {
public:
    class SpawnableMobInfo {
    public:
        int id;
        eMinecraftColour eggColor1;
        eMinecraftColour eggColor2;
        int nameId;  // 4J Added

        SpawnableMobInfo(int id, eMinecraftColour eggColor1,
                         eMinecraftColour eggColor2, int nameId) {
            this->id = id;
            this->eggColor1 = eggColor1;
            this->eggColor2 = eggColor2;
            this->nameId = nameId;
        }
    };

private:
    static std::unordered_map<std::string, entityCreateFn>* idCreateMap;
    static std::unordered_map<eINSTANCEOF, std::string, eINSTANCEOFKeyHash,
                              eINSTANCEOFKeyEq>* classIdMap;
    static std::unordered_map<int, entityCreateFn>* numCreateMap;
    static std::unordered_map<int, eINSTANCEOF>* numClassMap;
    static std::unordered_map<eINSTANCEOF, int, eINSTANCEOFKeyHash,
                              eINSTANCEOFKeyEq>* classNumMap;
    static std::unordered_map<std::string, int>* idNumMap;

public:
    static std::unordered_map<int, SpawnableMobInfo*> idsSpawnableInCreative;

private:
    static void setId(entityCreateFn createFn, eINSTANCEOF clas,
                      const std::string& id, int idNum);
    static void setId(entityCreateFn createFn, eINSTANCEOF clas,
                      const std::string& id, int idNum,
                      eMinecraftColour color1, eMinecraftColour color2,
                      int nameId);

public:
    static void staticCtor();
    static std::shared_ptr<Entity> newEntity(const std::string& id,
                                             Level* level);
    static std::shared_ptr<Entity> loadStatic(CompoundTag* tag, Level* level);
    static std::shared_ptr<Entity> newById(int id, Level* level);
    static std::shared_ptr<Entity> newByEnumType(eINSTANCEOF eType,
                                                 Level* level);
    static int getId(std::shared_ptr<Entity> entity);
    static std::string getEncodeId(std::shared_ptr<Entity> entity);
    static int getId(const std::string& encodeId);
    static std::string getEncodeId(int entityIoValue);
    static int getNameId(int entityIoValue);
    static eINSTANCEOF getType(const std::string& idString);
    static eINSTANCEOF getClass(int id);

    // 4J-JEV, added for enumerating mobs.
    static int eTypeToIoid(eINSTANCEOF eType);
};

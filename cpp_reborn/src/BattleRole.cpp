#include "BattleRole.h"
#include "GameManager.h"
#include "Role.h"

BattleRole::BattleRole() : GameObject(BATTLEROLE_DATA_SIZE) {}

int16 BattleRole::getLevel() const {
    // Get the underlying Role ID (rnum)
    int16 rnum = getRNum();
    // Access GameManager to get the Role object
    // Note: This creates a dependency on GameManager, which is fine for .cpp
    if (rnum >= 0) {
        return GameManager::getInstance().getRole(rnum).getLevel();
    }
    return 1; // Default level 1 if invalid
}

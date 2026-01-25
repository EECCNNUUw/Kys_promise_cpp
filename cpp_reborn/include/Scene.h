#pragma once
#include "GameObject.h"
#include <string>

constexpr size_t SCENE_DATA_SIZE = 26;

class Scene : public GameObject {
public:
    Scene() : GameObject(SCENE_DATA_SIZE) {}
    virtual ~Scene() = default;

    // 0: ListNum
    int16 getListNum() const { return m_data[0]; }

    // 1-5: Name (10 bytes = 5 words)
    std::string getName() const { return getString(1, 5); }

    // 6: ExitMusic
    int16 getExitMusic() const { return m_data[6]; }

    // 7: EntranceMusic
    int16 getEntranceMusic() const { return m_data[7]; }

    // ...
    
    // 10: MainEntranceY1
    int16 getMainEntranceY1() const { return m_data[10]; }
    // 11: MainEntranceX1
    int16 getMainEntranceX1() const { return m_data[11]; }
    
    // 14: EntranceY
    int16 getEntranceY() const { return m_data[14]; }
    // 15: EntranceX
    int16 getEntranceX() const { return m_data[15]; }
};

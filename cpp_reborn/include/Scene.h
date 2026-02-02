#pragma once
#include "GameObject.h"
#include <string>

constexpr size_t SCENE_DATA_SIZE = 27;

class Scene : public GameObject {
public:
    Scene() : GameObject(SCENE_DATA_SIZE) {}
    virtual ~Scene() = default;

    // 0: ListNum
    int16 getListNum() const { return m_data[0]; }

    // 1-10: Name (20 bytes = 10 words)
    std::string getName() const { return getString(1, 10); }

    // 11: ExitMusic
    int16 getExitMusic() const { return m_data[11]; }

    // 12: EntranceMusic
    int16 getEntranceMusic() const { return m_data[12]; }

    // 13: Pallet
    int16 getPallet() const { return m_data[13]; }
    
    // 14: EnCondition
    int16 getEnCondition() const { return m_data[14]; }
    
    // 15: MainEntranceY1
    int16 getMainEntranceY1() const { return m_data[15]; }
    // 16: MainEntranceX1
    int16 getMainEntranceX1() const { return m_data[16]; }
    
    // 17: MainEntranceY2
    int16 getMainEntranceY2() const { return m_data[17]; }
    // 18: MainEntranceX2
    int16 getMainEntranceX2() const { return m_data[18]; }
    
    // 19: EntranceY
    int16 getEntranceY() const { return m_data[19]; }
    // 20: EntranceX
    int16 getEntranceX() const { return m_data[20]; }
    
    // 21-26: ExitY, ExitX (3 exits, each has Y and X - Pascal order: ExitY first, then ExitX)
    int16 getExitX(int index) const { 
        if (index >= 0 && index < 3) return m_data[22 + index * 2];
        return -1;
    }
    int16 getExitY(int index) const { 
        if (index >= 0 && index < 3) return m_data[21 + index * 2];
        return -1;
    }
};

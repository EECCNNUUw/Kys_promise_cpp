#pragma once
#include "GameObject.h"
#include <string>

// 场景数据结构 - 对应 Pascal 原版 TScene
// TScene = record
//   case TCallType of
//     Element: (ListNum: smallint;
//       Name: array[0..9] of ansichar;
//       ExitMusic, EntranceMusic: smallint;
//       Pallet, EnCondition: smallint;
//       MainEntranceY1, MainEntranceX1, MainEntranceY2, MainEntranceX2: smallint;
//       EntranceY, EntranceX: smallint;
//       ExitY, ExitX: array[0..2] of smallint;
//       Mapmode, mapnum, useless3, useless4: smallint);
//     Address: (Data: array[0..25] of smallint);
// end;
constexpr size_t SCENE_DATA_SIZE = 26;

class Scene : public GameObject {
public:
    Scene() : GameObject(SCENE_DATA_SIZE) {}
    virtual ~Scene() = default;

    // 0: ListNum (列表编号)
    int16 getListNum() const { return m_data[0]; }
    void setListNum(int16 v) { m_data[0] = v; }

    // 1-5: Name (场景名称, 10 bytes = 5 int16s)
    std::string getName() const { return getString(1, 5); }
    void setName(const std::string& v) { setString(1, 5, v); }

    // 6: ExitMusic (离开时播放的音乐)
    int16 getExitMusic() const { return m_data[6]; }
    void setExitMusic(int16 v) { m_data[6] = v; }

    // 7: EntranceMusic (进入时播放的音乐)
    int16 getEntranceMusic() const { return m_data[7]; }
    void setEntranceMusic(int16 v) { m_data[7] = v; }

    // 8: Pallet (色调/配色方案)
    int16 getPallet() const { return m_data[8]; }
    void setPallet(int16 v) { m_data[8] = v; }

    // 9: EnCondition (进入条件)
    int16 getEnCondition() const { return m_data[9]; }
    void setEnCondition(int16 v) { m_data[9] = v; }

    // 10: MainEntranceY1 (主入口区域 Y1)
    int16 getMainEntranceY1() const { return m_data[10]; }
    void setMainEntranceY1(int16 v) { m_data[10] = v; }

    // 11: MainEntranceX1 (主入口区域 X1)
    int16 getMainEntranceX1() const { return m_data[11]; }
    void setMainEntranceX1(int16 v) { m_data[11] = v; }

    // 12: MainEntranceY2 (主入口区域 Y2)
    int16 getMainEntranceY2() const { return m_data[12]; }
    void setMainEntranceY2(int16 v) { m_data[12] = v; }

    // 13: MainEntranceX2 (主入口区域 X2)
    int16 getMainEntranceX2() const { return m_data[13]; }
    void setMainEntranceX2(int16 v) { m_data[13] = v; }

    // 14: EntranceY (入口 Y 坐标)
    int16 getEntranceY() const { return m_data[14]; }
    void setEntranceY(int16 v) { m_data[14] = v; }

    // 15: EntranceX (入口 X 坐标)
    int16 getEntranceX() const { return m_data[15]; }
    void setEntranceX(int16 v) { m_data[15] = v; }

    // 16-18: ExitY (出口 Y 坐标数组 array[0..2])
    int16 getExitY(int index) const { return (index >= 0 && index < 3) ? m_data[16 + index] : 0; }
    void setExitY(int index, int16 v) { if (index >= 0 && index < 3) m_data[16 + index] = v; }

    // 19-21: ExitX (出口 X 坐标数组 array[0..2])
    int16 getExitX(int index) const { return (index >= 0 && index < 3) ? m_data[19 + index] : 0; }
    void setExitX(int index, int16 v) { if (index >= 0 && index < 3) m_data[19 + index] = v; }

    // 22: Mapmode (地图模式: 0室内, 1室外)
    int16 getMapMode() const { return m_data[22]; }
    void setMapMode(int16 v) { m_data[22] = v; }

    // 23: mapnum (关联的地图编号)
    int16 getMapNum() const { return m_data[23]; }
    void setMapNum(int16 v) { m_data[23] = v; }

    // 24: useless3 (保留/未使用)
    int16 getUseless3() const { return m_data[24]; }
    void setUseless3(int16 v) { m_data[24] = v; }

    // 25: useless4 (保留/未使用)
    int16 getUseless4() const { return m_data[25]; }
    void setUseless4(int16 v) { m_data[25] = v; }
};

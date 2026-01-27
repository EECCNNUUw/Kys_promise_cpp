#ifndef WARDATA_H
#define WARDATA_H

#include "GameObject.h"
#include <vector>
#include <string>

// 战场数据结构 - 对应 Pascal 原版 TWarSta
// TWarSta = record
//   case TcallType of
//     Element: (BattleNum: smallint;
//       BattleName: array[0..9] of byte;
//       battlemap, exp, battlemusic: smallint;
//       mate, automate, mate_x, mate_y: array[0..11] of smallint;
//       enemy, enemy_x, enemy_y: array[0..29] of smallint;
//       BoutEvent, OperationEvent: smallint;
//       GetKongfu: array[0..2] of smallint;
//       GetItems: array[0..2] of smallint;
//       GetMoney: smallint);
//     Address: (Data: array[0..155] of smallint;)
// end;
constexpr size_t WARDATA_DATA_SIZE = 156;

class WarData : public GameObject {
public:
    WarData() : GameObject(WARDATA_DATA_SIZE) {}
    virtual ~WarData() = default;

    // 0: BattleNum (战斗编号)
    int16 getBattleNum() const { return m_data[0]; }
    void setBattleNum(int16 v) { m_data[0] = v; }

    // 1-5: BattleName (战斗名称, 10 bytes = 5 int16s)
    std::string getBattleName() const { return getString(1, 5); }
    void setBattleName(const std::string& v) { setString(1, 5, v); }

    // 6: BattleMap (战场地图编号)
    int16 getBattleMap() const { return m_data[6]; }
    void setBattleMap(int16 v) { m_data[6] = v; }

    // 7: Exp (经验值?)
    int16 getExp() const { return m_data[7]; }
    void setExp(int16 v) { m_data[7] = v; }

    // 8: BattleMusic (战斗音乐)
    int16 getBattleMusic() const { return m_data[8]; }
    void setBattleMusic(int16 v) { m_data[8] = v; }

    // 9-20: Mate (我方队友ID array[0..11])
    int16 getMate(int index) const { return (index >= 0 && index < 12) ? m_data[9 + index] : 0; }
    void setMate(int index, int16 v) { if (index >= 0 && index < 12) m_data[9 + index] = v; }

    // 21-32: AutoMate (自动战斗队友ID? array[0..11])
    int16 getAutoMate(int index) const { return (index >= 0 && index < 12) ? m_data[21 + index] : 0; }
    void setAutoMate(int index, int16 v) { if (index >= 0 && index < 12) m_data[21 + index] = v; }

    // 33-44: MateX (我方X坐标 array[0..11])
    int16 getMateX(int index) const { return (index >= 0 && index < 12) ? m_data[33 + index] : 0; }
    void setMateX(int index, int16 v) { if (index >= 0 && index < 12) m_data[33 + index] = v; }

    // 45-56: MateY (我方Y坐标 array[0..11])
    int16 getMateY(int index) const { return (index >= 0 && index < 12) ? m_data[45 + index] : 0; }
    void setMateY(int index, int16 v) { if (index >= 0 && index < 12) m_data[45 + index] = v; }

    // 57-86: Enemy (敌方ID array[0..29])
    int16 getEnemy(int index) const { return (index >= 0 && index < 30) ? m_data[57 + index] : 0; }
    void setEnemy(int index, int16 v) { if (index >= 0 && index < 30) m_data[57 + index] = v; }

    // 87-116: EnemyX (敌方X坐标 array[0..29])
    int16 getEnemyX(int index) const { return (index >= 0 && index < 30) ? m_data[87 + index] : 0; }
    void setEnemyX(int index, int16 v) { if (index >= 0 && index < 30) m_data[87 + index] = v; }

    // 117-146: EnemyY (敌方Y坐标 array[0..29])
    int16 getEnemyY(int index) const { return (index >= 0 && index < 30) ? m_data[117 + index] : 0; }
    void setEnemyY(int index, int16 v) { if (index >= 0 && index < 30) m_data[117 + index] = v; }

    // 147: BoutEvent (回合事件?)
    int16 getBoutEvent() const { return m_data[147]; }
    void setBoutEvent(int16 v) { m_data[147] = v; }

    // 148: OperationEvent (操作事件?)
    int16 getOperationEvent() const { return m_data[148]; }
    void setOperationEvent(int16 v) { m_data[148] = v; }

    // 149-151: GetKongfu (获得武功 array[0..2])
    int16 getGetKongfu(int index) const { return (index >= 0 && index < 3) ? m_data[149 + index] : 0; }
    void setGetKongfu(int index, int16 v) { if (index >= 0 && index < 3) m_data[149 + index] = v; }

    // 152-154: GetItems (获得物品 array[0..2])
    int16 getGetItems(int index) const { return (index >= 0 && index < 3) ? m_data[152 + index] : 0; }
    void setGetItems(int index, int16 v) { if (index >= 0 && index < 3) m_data[152 + index] = v; }

    // 155: GetMoney (获得金钱)
    int16 getGetMoney() const { return m_data[155]; }
    void setGetMoney(int16 v) { m_data[155] = v; }
};

#endif // WARDATA_H

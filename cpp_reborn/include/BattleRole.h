#ifndef BATTLEROLE_H
#define BATTLEROLE_H

#include "GameObject.h"

// 战场角色数据结构 - 对应 Pascal 原版 TBattleRole
// TBattleRole = record
//   case TCallType of
//     Element: (rnum, Team, Y, X, Face, Dead, Step, Acted: smallint;
//       Pic, ShowNumber, Progress, Round, speed: smallint;
//       ExpGot, Auto, Show, wait, frozen, killed, Knowledge, LifeAdd: smallint;
//       AddAtt, AddDef, AddSpd, AddStep, AddDodge, PerfectDodge: smallint);
//     Address: (Data: array[0..26] of smallint);
// end;
constexpr size_t BATTLEROLE_DATA_SIZE = 27;

class BattleRole : public GameObject {
public:
    BattleRole();
    virtual ~BattleRole() = default;

    // 0: rnum (角色ID)
    int16 getRNum() const { return m_data[0]; }
    void setRNum(int16 v) { m_data[0] = v; }

    // 1: Team (队伍: 0我方, 1敌方, etc.)
    int16 getTeam() const { return m_data[1]; }
    void setTeam(int16 v) { m_data[1] = v; }

    // 2: Y (坐标Y)
    int16 getY() const { return m_data[2]; }
    void setY(int16 v) { m_data[2] = v; }

    // 3: X (坐标X)
    int16 getX() const { return m_data[3]; }
    void setX(int16 v) { m_data[3] = v; }

    // 4: Face (朝向)
    int16 getFace() const { return m_data[4]; }
    void setFace(int16 v) { m_data[4] = v; }

    // 5: Dead (是否死亡)
    int16 getDead() const { return m_data[5]; }
    void setDead(int16 v) { m_data[5] = v; }

    // 6: Step (已移动步数/状态?)
    int16 getStep() const { return m_data[6]; }
    void setStep(int16 v) { m_data[6] = v; }

    // 7: Acted (是否已行动)
    int16 getActed() const { return m_data[7]; }
    void setActed(int16 v) { m_data[7] = v; }

    // 8: Pic (贴图/动画帧)
    int16 getPic() const { return m_data[8]; }
    void setPic(int16 v) { m_data[8] = v; }

    // 9: ShowNumber (显示数字/伤害?)
    int16 getShowNumber() const { return m_data[9]; }
    void setShowNumber(int16 v) { m_data[9] = v; }

    // 10: Progress (集气槽进度)
    int16 getProgress() const { return m_data[10]; }
    void setProgress(int16 v) { m_data[10] = v; }

    // 11: Round (回合数)
    int16 getRound() const { return m_data[11]; }
    void setRound(int16 v) { m_data[11] = v; }

    // 12: speed (速度 - 战斗中可能变化)
    int16 getSpeed() const { return m_data[12]; }
    void setSpeed(int16 v) { m_data[12] = v; }

    // 13: ExpGot (获得经验)
    int16 getExpGot() const { return m_data[13]; }
    void setExpGot(int16 v) { m_data[13] = v; }

    // 14: Auto (自动战斗状态)
    int16 getAuto() const { return m_data[14]; }
    void setAuto(int16 v) { m_data[14] = v; }

    // 15: Show (是否显示?)
    int16 getShow() const { return m_data[15]; }
    void setShow(int16 v) { m_data[15] = v; }

    // 16: wait (等待时间?)
    int16 getWait() const { return m_data[16]; }
    void setWait(int16 v) { m_data[16] = v; }

    // 17: frozen (冻结/冰封状态)
    int16 getFrozen() const { return m_data[17]; }
    void setFrozen(int16 v) { m_data[17] = v; }

    // 18: killed (击杀数?)
    int16 getKilled() const { return m_data[18]; }
    void setKilled(int16 v) { m_data[18] = v; }

    // 19: Knowledge (见识/知识 - 战斗中可能用到?)
    int16 getKnowledge() const { return m_data[19]; }
    void setKnowledge(int16 v) { m_data[19] = v; }

    // 20: LifeAdd (生命回复?)
    int16 getLifeAdd() const { return m_data[20]; }
    void setLifeAdd(int16 v) { m_data[20] = v; }

    // 21: AddAtt (攻击加成)
    int16 getAddAtt() const { return m_data[21]; }
    void setAddAtt(int16 v) { m_data[21] = v; }

    // 22: AddDef (防御加成)
    int16 getAddDef() const { return m_data[22]; }
    void setAddDef(int16 v) { m_data[22] = v; }

    // 23: AddSpd (速度加成)
    int16 getAddSpd() const { return m_data[23]; }
    void setAddSpd(int16 v) { m_data[23] = v; }

    // 24: AddStep (移动步数加成)
    int16 getAddStep() const { return m_data[24]; }
    void setAddStep(int16 v) { m_data[24] = v; }

    // 25: AddDodge (闪避加成?)
    int16 getAddDodge() const { return m_data[25]; }
    void setAddDodge(int16 v) { m_data[25] = v; }

    // 26: PerfectDodge (完美闪避?)
    int16 getPerfectDodge() const { return m_data[26]; }
    void setPerfectDodge(int16 v) { m_data[26] = v; }

    // Helper to get level from underlying Role (Implemented in cpp due to dependency)
    // 获取角色等级 (实现位于cpp文件中，因为依赖GameManager)
    int16 getLevel() const;
};

#endif // BATTLEROLE_H

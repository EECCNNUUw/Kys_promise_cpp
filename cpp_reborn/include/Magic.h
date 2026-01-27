#pragma once
#include "GameObject.h"
#include <string>

// Mapping TMagic from kys_main.pas
// Size: 111 smallints
// 武功数据结构 - 对应 Pascal 原版 TMagic
constexpr size_t MAGIC_DATA_SIZE = 111;

class Magic : public GameObject {
public:
    Magic() : GameObject(MAGIC_DATA_SIZE) {}
    virtual ~Magic() = default;

    // --- Property Accessors based on Pascal Record Layout ---
    // 注意：偏移量以 16-bit word (int16) 为单位

    // 0: ListNum (列表编号)
    int16 getListNum() const { return m_data[0]; }
    void setListNum(int16 v) { m_data[0] = v; }

    // 1-5: Name (武功名称, 10 bytes = 5 int16s)
    std::string getName() const { return getString(1, 5); }
    void setName(const std::string& v) { setString(1, 5, v); }

    // 6: Useless (保留/未使用)
    int16 getUseless() const { return m_data[6]; }
    void setUseless(int16 v) { m_data[6] = v; }

    // 7: NeedHP (需要生命值?)
    int16 getNeedHP() const { return m_data[7]; }
    void setNeedHP(int16 v) { m_data[7] = v; }

    // 8: MinStep (最小步数?)
    int16 getMinStep() const { return m_data[8]; }
    void setMinStep(int16 v) { m_data[8] = v; }

    // 9: BigAmi (大动画编号?)
    int16 getBigAmi() const { return m_data[9]; }
    void setBigAmi(int16 v) { m_data[9] = v; }

    // 10: EventNum (关联事件)
    int16 getEventNum() const { return m_data[10]; }
    void setEventNum(int16 v) { m_data[10] = v; }
    
    // 11: SoundNum (音效编号)
    int16 getSoundNum() const { return m_data[11]; }
    void setSoundNum(int16 v) { m_data[11] = v; }

    // 12: MagicType (武功类型)
    int16 getMagicType() const { return m_data[12]; }
    void setMagicType(int16 v) { m_data[12] = v; }

    // 13: AmiNum (动画编号)
    int16 getAmiNum() const { return m_data[13]; }
    void setAmiNum(int16 v) { m_data[13] = v; }

    // 14: HurtType (伤害类型)
    int16 getHurtType() const { return m_data[14]; }
    void setHurtType(int16 v) { m_data[14] = v; }

    // 15: AttAreaType (攻击范围类型)
    int16 getAttAreaType() const { return m_data[15]; }
    void setAttAreaType(int16 v) { m_data[15] = v; }

    // 16: NeedMP (需要内力)
    int16 getNeedMP() const { return m_data[16]; }
    void setNeedMP(int16 v) { m_data[16] = v; }

    // 17: Poision (毒性/中毒)
    int16 getPoision() const { return m_data[17]; }
    void setPoision(int16 v) { m_data[17] = v; }

    // 18: MinHurt (最小伤害)
    int16 getMinHurt() const { return m_data[18]; }
    void setMinHurt(int16 v) { m_data[18] = v; }

    // 19: MaxHurt (最大伤害)
    int16 getMaxHurt() const { return m_data[19]; }
    void setMaxHurt(int16 v) { m_data[19] = v; }

    // 20: HurtModulus (伤害系数)
    int16 getHurtModulus() const { return m_data[20]; }
    void setHurtModulus(int16 v) { m_data[20] = v; }

    // 21: AttackModulus (攻击力影响系数)
    int16 getAttackModulus() const { return m_data[21]; }
    void setAttackModulus(int16 v) { m_data[21] = v; }

    // 22: MPModulus (内力影响系数)
    int16 getMPModulus() const { return m_data[22]; }
    void setMPModulus(int16 v) { m_data[22] = v; }

    // 23: SpeedModulus (轻功影响系数)
    int16 getSpeedModulus() const { return m_data[23]; }
    void setSpeedModulus(int16 v) { m_data[23] = v; }

    // 24: WeaponModulus (武器影响系数)
    int16 getWeaponModulus() const { return m_data[24]; }
    void setWeaponModulus(int16 v) { m_data[24] = v; }

    // 25: NeedProgress (需要进度?)
    int16 getNeedProgress() const { return m_data[25]; }
    void setNeedProgress(int16 v) { m_data[25] = v; }

    // 26: AddMpScale (吸内比例?)
    int16 getAddMpScale() const { return m_data[26]; }
    void setAddMpScale(int16 v) { m_data[26] = v; }

    // 27: AddHpScale (吸血比例?)
    int16 getAddHpScale() const { return m_data[27]; }
    void setAddHpScale(int16 v) { m_data[27] = v; }

    // 28-37: MoveDistance (移动距离/范围 array[0..9]) - 对应不同等级
    int16 getMoveDistance(int level) const { return (level >= 0 && level < 10) ? m_data[28 + level] : 0; }
    void setMoveDistance(int level, int16 v) { if (level >= 0 && level < 10) m_data[28 + level] = v; }

    // 38-47: AttDistance (攻击距离/范围 array[0..9]) - 对应不同等级
    int16 getAttDistance(int level) const { return (level >= 0 && level < 10) ? m_data[38 + level] : 0; }
    void setAttDistance(int level, int16 v) { if (level >= 0 && level < 10) m_data[38 + level] = v; }
    
    // 48-50: AddHP (消耗/增加生命 array[0..2])
    int16 getAddHP(int index) const { return (index >= 0 && index < 3) ? m_data[48 + index] : 0; }
    void setAddHP(int index, int16 v) { if (index >= 0 && index < 3) m_data[48 + index] = v; }

    // 51-53: AddMP (消耗/增加内力 array[0..2])
    int16 getAddMP(int index) const { return (index >= 0 && index < 3) ? m_data[51 + index] : 0; }
    void setAddMP(int index, int16 v) { if (index >= 0 && index < 3) m_data[51 + index] = v; }

    // 54-56: AddAtt (增加攻击力 array[0..2])
    int16 getAddAtt(int index) const { return (index >= 0 && index < 3) ? m_data[54 + index] : 0; }
    void setAddAtt(int index, int16 v) { if (index >= 0 && index < 3) m_data[54 + index] = v; }

    // 57-59: AddDef (增加防御力 array[0..2])
    int16 getAddDef(int index) const { return (index >= 0 && index < 3) ? m_data[57 + index] : 0; }
    void setAddDef(int index, int16 v) { if (index >= 0 && index < 3) m_data[57 + index] = v; }

    // 60-62: AddSpd (增加轻功 array[0..2])
    int16 getAddSpd(int index) const { return (index >= 0 && index < 3) ? m_data[60 + index] : 0; }
    void setAddSpd(int index, int16 v) { if (index >= 0 && index < 3) m_data[60 + index] = v; }

    // 63: MinPeg (最小走火入魔?)
    int16 getMinPeg() const { return m_data[63]; }
    void setMinPeg(int16 v) { m_data[63] = v; }

    // 64: MaxPeg (最大走火入魔?)
    int16 getMaxPeg() const { return m_data[64]; }
    void setMaxPeg(int16 v) { m_data[64] = v; }

    // 65: MinInjury (最小内伤?)
    int16 getMinInjury() const { return m_data[65]; }
    void setMinInjury(int16 v) { m_data[65] = v; }

    // 66: MaxInjury (最大内伤?)
    int16 getMaxInjury() const { return m_data[66]; }
    void setMaxInjury(int16 v) { m_data[66] = v; }

    // 67: AddMedcine (增加医疗)
    int16 getAddMedcine() const { return m_data[67]; }
    void setAddMedcine(int16 v) { m_data[67] = v; }

    // 68: AddUsePoi (增加用毒)
    int16 getAddUsePoi() const { return m_data[68]; }
    void setAddUsePoi(int16 v) { m_data[68] = v; }

    // 69: AddMedPoi (增加解毒)
    int16 getAddMedPoi() const { return m_data[69]; }
    void setAddMedPoi(int16 v) { m_data[69] = v; }

    // 70: AddDefPoi (增加抗毒)
    int16 getAddDefPoi() const { return m_data[70]; }
    void setAddDefPoi(int16 v) { m_data[70] = v; }

    // 71: AddFist (增加拳掌)
    int16 getAddFist() const { return m_data[71]; }
    void setAddFist(int16 v) { m_data[71] = v; }

    // 72: AddSword (增加御剑)
    int16 getAddSword() const { return m_data[72]; }
    void setAddSword(int16 v) { m_data[72] = v; }

    // 73: AddKnife (增加耍刀)
    int16 getAddKnife() const { return m_data[73]; }
    void setAddKnife(int16 v) { m_data[73] = v; }

    // 74: AddUnusual (增加特殊兵器)
    int16 getAddUnusual() const { return m_data[74]; }
    void setAddUnusual(int16 v) { m_data[74] = v; }

    // 75: AddHidWeapon (增加暗器)
    int16 getAddHidWeapon() const { return m_data[75]; }
    void setAddHidWeapon(int16 v) { m_data[75] = v; }

    // 76: BattleState (战斗状态?)
    int16 getBattleState() const { return m_data[76]; }
    void setBattleState(int16 v) { m_data[76] = v; }

    // 77-79: NeedExp (需要经验 array[0..2])
    int16 getNeedExp(int index) const { return (index >= 0 && index < 3) ? m_data[77 + index] : 0; }
    void setNeedExp(int index, int16 v) { if (index >= 0 && index < 3) m_data[77 + index] = v; }

    // 80: MaxLevel (最大等级)
    int16 getMaxLevel() const { return m_data[80]; }
    void setMaxLevel(int16 v) { m_data[80] = v; }

    // 81-110: Introduction (武功说明, 60 bytes = 30 int16s)
    std::string getIntroduction() const { return getString(81, 30); }
    void setIntroduction(const std::string& v) { setString(81, 30, v); }
};

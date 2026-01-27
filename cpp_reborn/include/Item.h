#pragma once
#include "GameObject.h"
#include <string>

// Mapping TItem from kys_main.pas
// Size: 95 smallints (Address: Data: array[0..94] of smallint)
// 物品数据结构 - 对应 Pascal 原版 TItem
constexpr size_t ITEM_DATA_SIZE = 95;

class Item : public GameObject {
public:
    Item() : GameObject(ITEM_DATA_SIZE) {}
    virtual ~Item() = default;

    // --- Property Accessors based on Pascal Record Layout ---
    // 注意：偏移量以 16-bit word (int16) 为单位

    // 0: ListNum (列表编号)
    int16 getListNum() const { return m_data[0]; }
    void setListNum(int16 v) { m_data[0] = v; }

    // 1-10: Name (物品名称, 20 bytes = 10 int16s)
    std::string getName() const { return getString(1, 10); }
    void setName(const std::string& v) { setString(1, 10, v); }

    // 11: ExpOfMagic (武功经验/配合?)
    int16 getExpOfMagic() const { return m_data[11]; }
    void setExpOfMagic(int16 v) { m_data[11] = v; }

    // 12: SetNum (套装编号)
    int16 getSetNum() const { return m_data[12]; }
    void setSetNum(int16 v) { m_data[12] = v; }

    // 13: BattleEffect (战斗特效)
    int16 getBattleEffect() const { return m_data[13]; }
    void setBattleEffect(int16 v) { m_data[13] = v; }

    // 14: WineEffect (酒效力)
    int16 getWineEffect() const { return m_data[14]; }
    void setWineEffect(int16 v) { m_data[14] = v; }

    // 15: needSex (性别要求)
    int16 getNeedSex() const { return m_data[15]; }
    void setNeedSex(int16 v) { m_data[15] = v; }

    // 16-20: unuse (未使用/保留 array[0..4])
    int16 getUnuse(int index) const { return (index >= 0 && index < 5) ? m_data[16 + index] : 0; }
    void setUnuse(int index, int16 v) { if (index >= 0 && index < 5) m_data[16 + index] = v; }

    // 21-35: Introduction (物品说明, 30 bytes = 15 int16s)
    std::string getIntroduction() const { return getString(21, 15); }
    void setIntroduction(const std::string& v) { setString(21, 15, v); }

    // 36: Magic (关联武功ID)
    int16 getMagic() const { return m_data[36]; }
    void setMagic(int16 v) { m_data[36] = v; }

    // 37: AmiNum (动画编号)
    int16 getAmiNum() const { return m_data[37]; }
    void setAmiNum(int16 v) { m_data[37] = v; }

    // 38: User (专属用户ID)
    int16 getUser() const { return m_data[38]; }
    void setUser(int16 v) { m_data[38] = v; }

    // 39: EquipType (装备类型)
    int16 getEquipType() const { return m_data[39]; }
    void setEquipType(int16 v) { m_data[39] = v; }

    // 40: ShowIntro (显示说明标志)
    int16 getShowIntro() const { return m_data[40]; }
    void setShowIntro(int16 v) { m_data[40] = v; }

    // 41: ItemType (物品类型)
    int16 getItemType() const { return m_data[41]; }
    void setItemType(int16 v) { m_data[41] = v; }

    // 42: inventory (库存/数量?)
    int16 getInventory() const { return m_data[42]; }
    void setInventory(int16 v) { m_data[42] = v; }

    // 43: price (价格)
    int16 getPrice() const { return m_data[43]; }
    void setPrice(int16 v) { m_data[43] = v; }

    // 44: EventNum (关联事件编号)
    int16 getEventNum() const { return m_data[44]; }
    void setEventNum(int16 v) { m_data[44] = v; }

    // 45: AddCurrentHP (增加当前生命)
    int16 getAddCurrentHP() const { return m_data[45]; }
    void setAddCurrentHP(int16 v) { m_data[45] = v; }

    // 46: AddMaxHP (增加最大生命)
    int16 getAddMaxHP() const { return m_data[46]; }
    void setAddMaxHP(int16 v) { m_data[46] = v; }

    // 47: AddPoi (增加中毒/带毒?)
    int16 getAddPoi() const { return m_data[47]; }
    void setAddPoi(int16 v) { m_data[47] = v; }

    // 48: AddPhyPower (增加体力)
    int16 getAddPhyPower() const { return m_data[48]; }
    void setAddPhyPower(int16 v) { m_data[48] = v; }

    // 49: ChangeMPType (改变内力属性)
    int16 getChangeMPType() const { return m_data[49]; }
    void setChangeMPType(int16 v) { m_data[49] = v; }

    // 50: AddCurrentMP (增加当前内力)
    int16 getAddCurrentMP() const { return m_data[50]; }
    void setAddCurrentMP(int16 v) { m_data[50] = v; }

    // 51: AddMaxMP (增加最大内力)
    int16 getAddMaxMP() const { return m_data[51]; }
    void setAddMaxMP(int16 v) { m_data[51] = v; }

    // 52: AddAttack (增加攻击力)
    int16 getAddAttack() const { return m_data[52]; }
    void setAddAttack(int16 v) { m_data[52] = v; }

    // 53: AddSpeed (增加轻功)
    int16 getAddSpeed() const { return m_data[53]; }
    void setAddSpeed(int16 v) { m_data[53] = v; }

    // 54: AddDefence (增加防御力)
    int16 getAddDefence() const { return m_data[54]; }
    void setAddDefence(int16 v) { m_data[54] = v; }

    // 55: AddMedcine (增加医疗)
    int16 getAddMedcine() const { return m_data[55]; }
    void setAddMedcine(int16 v) { m_data[55] = v; }

    // 56: AddUsePoi (增加用毒)
    int16 getAddUsePoi() const { return m_data[56]; }
    void setAddUsePoi(int16 v) { m_data[56] = v; }

    // 57: AddMedPoi (增加解毒)
    int16 getAddMedPoi() const { return m_data[57]; }
    void setAddMedPoi(int16 v) { m_data[57] = v; }

    // 58: AddDefPoi (增加抗毒)
    int16 getAddDefPoi() const { return m_data[58]; }
    void setAddDefPoi(int16 v) { m_data[58] = v; }

    // 59: AddFist (增加拳掌)
    int16 getAddFist() const { return m_data[59]; }
    void setAddFist(int16 v) { m_data[59] = v; }

    // 60: AddSword (增加御剑)
    int16 getAddSword() const { return m_data[60]; }
    void setAddSword(int16 v) { m_data[60] = v; }

    // 61: AddKnife (增加耍刀)
    int16 getAddKnife() const { return m_data[61]; }
    void setAddKnife(int16 v) { m_data[61] = v; }

    // 62: AddUnusual (增加特殊兵器)
    int16 getAddUnusual() const { return m_data[62]; }
    void setAddUnusual(int16 v) { m_data[62] = v; }

    // 63: AddHidWeapon (增加暗器)
    int16 getAddHidWeapon() const { return m_data[63]; }
    void setAddHidWeapon(int16 v) { m_data[63] = v; }

    // 64: AddKnowledge (增加武学常识)
    int16 getAddKnowledge() const { return m_data[64]; }
    void setAddKnowledge(int16 v) { m_data[64] = v; }

    // 65: AddEthics (增加品德)
    int16 getAddEthics() const { return m_data[65]; }
    void setAddEthics(int16 v) { m_data[65] = v; }

    // 66: AddAttTwice (增加左右互搏)
    int16 getAddAttTwice() const { return m_data[66]; }
    void setAddAttTwice(int16 v) { m_data[66] = v; }

    // 67: AddAttPoi (增加攻击带毒)
    int16 getAddAttPoi() const { return m_data[67]; }
    void setAddAttPoi(int16 v) { m_data[67] = v; }

    // 68: OnlyPracRole (仅限修炼角色? 还是 OnlySuitableRole?)
    // Pascal: OnlyPracRole
    int16 getOnlyPracRole() const { return m_data[68]; }
    void setOnlyPracRole(int16 v) { m_data[68] = v; }

    // 69: NeedMPType (修炼/装备需要内力属性)
    int16 getNeedMPType() const { return m_data[69]; }
    void setNeedMPType(int16 v) { m_data[69] = v; }

    // 70: NeedMP (需要内力数值)
    int16 getNeedMP() const { return m_data[70]; }
    void setNeedMP(int16 v) { m_data[70] = v; }

    // 71: NeedAttack (需要攻击力)
    int16 getNeedAttack() const { return m_data[71]; }
    void setNeedAttack(int16 v) { m_data[71] = v; }

    // 72: NeedSpeed (需要轻功)
    int16 getNeedSpeed() const { return m_data[72]; }
    void setNeedSpeed(int16 v) { m_data[72] = v; }

    // 73: NeedUsePoi (需要用毒)
    int16 getNeedUsePoi() const { return m_data[73]; }
    void setNeedUsePoi(int16 v) { m_data[73] = v; }

    // 74: NeedMedcine (需要医疗)
    int16 getNeedMedcine() const { return m_data[74]; }
    void setNeedMedcine(int16 v) { m_data[74] = v; }

    // 75: NeedMedPoi (需要解毒)
    int16 getNeedMedPoi() const { return m_data[75]; }
    void setNeedMedPoi(int16 v) { m_data[75] = v; }

    // 76: NeedFist (需要拳掌)
    int16 getNeedFist() const { return m_data[76]; }
    void setNeedFist(int16 v) { m_data[76] = v; }

    // 77: NeedSword (需要御剑)
    int16 getNeedSword() const { return m_data[77]; }
    void setNeedSword(int16 v) { m_data[77] = v; }

    // 78: NeedKnife (需要耍刀)
    int16 getNeedKnife() const { return m_data[78]; }
    void setNeedKnife(int16 v) { m_data[78] = v; }

    // 79: NeedUnusual (需要特殊兵器)
    int16 getNeedUnusual() const { return m_data[79]; }
    void setNeedUnusual(int16 v) { m_data[79] = v; }

    // 80: NeedHidWeapon (需要暗器)
    int16 getNeedHidWeapon() const { return m_data[80]; }
    void setNeedHidWeapon(int16 v) { m_data[80] = v; }

    // 81: NeedAptitude (需要资质)
    int16 getNeedAptitude() const { return m_data[81]; }
    void setNeedAptitude(int16 v) { m_data[81] = v; }

    // 82: NeedExp (修炼需要经验点)
    int16 getNeedExp() const { return m_data[82]; }
    void setNeedExp(int16 v) { m_data[82] = v; }

    // 83: Count (数量/计数?)
    int16 getCount() const { return m_data[83]; }
    void setCount(int16 v) { m_data[83] = v; }

    // 84: Rate (概率/比率?)
    int16 getRate() const { return m_data[84]; }
    void setRate(int16 v) { m_data[84] = v; }

    // 85-89: NeedItem (制造/修炼需要物品 array[0..4])
    int16 getNeedItem(int index) const { return (index >= 0 && index < 5) ? m_data[85 + index] : 0; }
    void setNeedItem(int index, int16 v) { if (index >= 0 && index < 5) m_data[85 + index] = v; }

    // 90-94: NeedMatAmount (需要物品数量/材料数量 array[0..4])
    int16 getNeedMatAmount(int index) const { return (index >= 0 && index < 5) ? m_data[90 + index] : 0; }
    void setNeedMatAmount(int index, int16 v) { if (index >= 0 && index < 5) m_data[90 + index] = v; }
};

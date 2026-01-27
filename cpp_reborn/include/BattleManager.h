#ifndef BATTLEMANAGER_H
#define BATTLEMANAGER_H

#include <vector>
#include <cstdint>
#include <string>
#include "BattleRole.h"
#include "WarData.h"

class BattleManager {
public:
    static BattleManager& getInstance();

    // Initialize battle system
    // 初始化战斗系统
    bool Init();

    // Start a specific battle
    // 开始一场战斗，对应 Pascal 的 Battle 函数
    // battleId: 战斗编号 (0-based)
    bool StartBattle(int battleId);

    // Main battle loop (blocking)
    // 战斗主循环，对应 Pascal 的 BattleMainControl
    void RunBattle();

    // Reset battle state
    // 重置战斗状态，对应 Pascal 的 InitialBField 部分逻辑
    void ResetBattle();

    // Load battle field map
    // 加载战场地图，对应 Pascal 的 InitialBField 读取 warfld.idx/grp
    bool LoadBattleField(int fieldNum);

    // Accessors
    BattleRole& getBattleRole(int index);
    int getBattleRoleCount() const;
    int16_t getBattleField(int layer, int x, int y) const;
    void setBattleField(int layer, int x, int y, int16_t val); // Added for testing
    
    // Testing Helper
    void AddBattleRole(const BattleRole& role);
    void ClearBattleRoles();

    // 移动角色，对应 Pascal 的 MoveRole
    void MoveRole(int roleIdx, int x, int y);
    // 攻击，对应 Pascal 的 AttackAction
    void Attack(int roleIdx, int targetIdx, int magicId);
    
    // BFS Movement Range
    // Fills m_battleField[3] with step counts (-1 if unreachable)
    // 计算移动范围，对应 Pascal 的 CalMoveAbility / SeekPath
    void CalSelectableArea(int roleIdx);

    // Damage Calculation
    // 计算伤害值，对应 Pascal 的 CalHurtValue
    int CalHurtValue(int attackerIdx, int targetIdx, int magicId, int level);

    // Battle Menu
    // 战斗菜单，对应 Pascal 的 BattleMenu
    int BattleMenu(int roleIdx);
    void BattleMenuItem(int roleIdx);

    // Target Selection
    bool SelectFriendlyRole(int roleIdx);
    // 自动战斗逻辑，对应 Pascal 的 AutoBattle
    void SelectAuto(int roleIdx);
    
    // New Selection Helpers
    bool SelectMove(int roleIdx, int& outX, int& outY);
    bool SelectAttack(int roleIdx, int& outTargetIdx);

    // Visual Effects
    // 显示伤害数值，对应 Pascal 的 ShowHurtValue
    void ShowHurtValue(int mode); // 0:Red, 1:Purple, 2:Green, 3:Blue, 4:Cyan
    void ShowHurtValue(const std::string& str, uint32_t color1, uint32_t color2);
    // 播放武功动画，对应 Pascal 的 PlayMagicAmination
    void PlayMagicAmination(int bnum, int magicId, int level);
    
    // Actions
    // 医疗，对应 Pascal 的 Medcine
    void Medcine(int roleIdx);
    // 解毒，对应 Pascal 的 MedPoision
    void MedFrozen(int roleIdx); // Note: Name might be misleading, check implementation
    void MedPoision(int roleIdx);
    // 用毒，对应 Pascal 的 UsePoision
    void UsePoision(int roleIdx);
    // 暗器，对应 Pascal 的 UseHiddenWeapen
    void UseHiddenWeapen(int roleIdx, int itemIdx);
    // 自动使用物品，对应 Pascal 的 AutoUseItem
    void AutoUseItem(int roleIdx, int listType); // listType: 45=HP, 50=MP, 48=Phy
    
    // Logic Helpers (Public for now, or friend)
    void ApplyMedicine(int healerIdx, int targetIdx);
    void ApplyMedFrozen(int healerIdx, int targetIdx);
    void ApplyMedPoision(int healerIdx, int targetIdx);
    void ApplyUsePoision(int attackerIdx, int targetIdx);
    void ApplyHiddenWeapon(int attackerIdx, int targetIdx, int itemIdx);
    
    // Area Logic
    // 设置攻击范围，对应 Pascal 的 SelectRange/SelectDirector 等
    void SetAttackArea(int type, int x, int y, int range, int bx, int by, int step);

    // Core Logic
    // 计算并应用伤害，对应 Pascal 的 CalHurtRole
    void CalHurtRole(int attackerIdx, int magicId, int level);
    // 计算中毒伤害，对应 Pascal 的 CalPoiHurtLife
    void CalPoiHurtLife(int roleIdx);

private:
    BattleManager();
    ~BattleManager() = default;
    BattleManager(const BattleManager&) = delete;
    BattleManager& operator=(const BattleManager&) = delete;

    // 计算战场进度，对应 Pascal 的 CountProgress
    int CountProgress();
    // 计算移动能力，对应 Pascal 的 CalMoveAbility
    void CalMoveAbility();
    // 自动战斗逻辑，对应 Pascal 的 AutoBattle
    void AutoBattle(int roleIdx);

    // UI Helpers
    void ShowBMenu(int menuStatus, int menu, int max);
    void RenderBattle(); // Draws map and roles
    void ShowItemMenu(const std::vector<int>& itemIds, int current, int x, int y);
    void ApplyItemEffect(int rnum, int inum, int where = 0);
    // void ApplyMedicine(int healerRoleIdx, int targetRoleIdx); // Moved to public

    // Selection Helpers
    bool SelectAim(int roleIdx, int step);
    int SelectAutoMode();
    bool TeamModeMenu();
    void ShowModeMenu(int menu);
    void ShowTeamModeMenu(int menu);
    void DrawBFieldWithCursor(int attAreaType, int step, int range);

    // Load War.sta data for a specific battle
    // 加载 War.sta 数据，对应 Pascal 的 InitialBField 读取 war.sta
    bool LoadWarData(int battleId);

    // Battle Roles (BRole array)
    // 战斗角色列表，对应 Pascal 的 Brole 数组
    std::vector<BattleRole> m_battleRoles;
    
    // Battle Field Map (BField array)
    // 8 layers, 64x64 grid
    // Layers: 0:Ground, 1:Building, 2:Roles, 3:Selectable?, 4:AttackRange?, etc.
    // 战场地图数据，对应 Pascal 的 BField 数组 (3维数组 [layer, x, y])
    int16_t m_battleField[8][64][64];

    // Current War Data
    // 当前战斗配置数据，对应 Pascal 的 warsta 结构
    WarData m_warData;

    // Helper to read War.sta file
    std::string m_warStaPath;
    std::string m_warFldIdxPath;
    std::string m_warFldGrpPath;

    // Runtime State
    bool m_battleRunning;
    int m_currentRoleIndex;
    int m_cursorX;
    int m_cursorY;
};

#endif // BATTLEMANAGER_H

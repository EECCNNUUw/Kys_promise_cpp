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
    bool Init();

    // Start a specific battle
    bool StartBattle(int battleId);

    // Main battle loop (blocking)
    void RunBattle();

    // Reset battle state
    void ResetBattle();

    // Load battle field map
    bool LoadBattleField(int fieldNum);

    // Accessors
    BattleRole& getBattleRole(int index);
    int getBattleRoleCount() const;
    int16_t getBattleField(int layer, int x, int y) const;
    void setBattleField(int layer, int x, int y, int16_t val); // Added for testing
    
    // Testing Helper
    void AddBattleRole(const BattleRole& role);
    void ClearBattleRoles();

    void MoveRole(int roleIdx, int x, int y);
    void Attack(int roleIdx, int targetIdx, int magicId);
    
    // BFS Movement Range
    // Fills m_battleField[3] with step counts (-1 if unreachable)
    void CalSelectableArea(int roleIdx);

    // Damage Calculation
    int CalHurtValue(int attackerIdx, int targetIdx, int magicId, int level);

    // Battle Menu
    int BattleMenu(int roleIdx);
    void BattleMenuItem(int roleIdx);

    // Target Selection
    bool SelectFriendlyRole(int roleIdx);
    void SelectAuto(int roleIdx);
    
    // New Selection Helpers
    bool SelectMove(int roleIdx, int& outX, int& outY);
    bool SelectAttack(int roleIdx, int& outTargetIdx);

    // Visual Effects
    void ShowHurtValue(int mode); // 0:Red, 1:Purple, 2:Green, 3:Blue, 4:Cyan
    void ShowHurtValue(const std::string& str, uint32_t color1, uint32_t color2);
    void PlayMagicAmination(int bnum, int magicId, int level);
    
    // Actions
    void Medcine(int roleIdx);
    void MedFrozen(int roleIdx);
    void MedPoision(int roleIdx);
    void UsePoision(int roleIdx);
    void UseHiddenWeapen(int roleIdx, int itemIdx);
    void AutoUseItem(int roleIdx, int listType); // listType: 45=HP, 50=MP, 48=Phy
    
    // Logic Helpers (Public for now, or friend)
    void ApplyMedicine(int healerIdx, int targetIdx);
    void ApplyMedFrozen(int healerIdx, int targetIdx);
    void ApplyMedPoision(int healerIdx, int targetIdx);
    void ApplyUsePoision(int attackerIdx, int targetIdx);
    void ApplyHiddenWeapon(int attackerIdx, int targetIdx, int itemIdx);
    
    // Area Logic
    void SetAttackArea(int type, int x, int y, int range, int bx, int by, int step);

    // Core Logic
    void CalHurtRole(int attackerIdx, int magicId, int level);
    void CalPoiHurtLife(int roleIdx);

private:
    BattleManager();
    ~BattleManager() = default;
    BattleManager(const BattleManager&) = delete;
    BattleManager& operator=(const BattleManager&) = delete;

    int CountProgress();
    void CalMoveAbility();
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
    bool LoadWarData(int battleId);

    // Battle Roles (BRole array)
    std::vector<BattleRole> m_battleRoles;
    
    // Battle Field Map (BField array)
    // 8 layers, 64x64 grid
    // Layers: 0:Ground, 1:Building, 2:Roles, 3:Selectable?, 4:AttackRange?, etc.
    int16_t m_battleField[8][64][64];

    // Current War Data
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

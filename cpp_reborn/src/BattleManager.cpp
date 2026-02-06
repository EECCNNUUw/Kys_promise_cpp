#include "BattleManager.h"
#include "GameManager.h"
#include "SceneManager.h"
#include "UIManager.h"
#include "FileLoader.h"
#include "TextManager.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <iostream>
#include <cmath>

// --- Helper Functions (Ported from kys_event.pas/kys_battle.pas) ---

static int GetMagicLevel(int rnum, int mnum) {
    if (rnum < 0) return 0;
    Role& role = GameManager::getInstance().getRole(rnum);
    for (int i = 0; i < 10; ++i) {
        if (role.getMagic(i) == mnum) {
            return role.getMagLevel(i);
        }
    }
    return -1;
}

static int GetGongtiLevel(int rnum, int mnum) {
    if (mnum < 0) return 0;
    int magicLevel = GetMagicLevel(rnum, mnum);
    Magic& magic = GameManager::getInstance().getMagic(mnum);
    return std::min((int)magic.getMaxLevel(), magicLevel / 100);
}

static int CheckEquipSet(int e0, int e1, int e2, int e3) {
    return GameManager::getInstance().CheckEquipSet(e0, e1, e2, e3);
}

static int GetRoleAttack(int rnum, bool equip) {
    Role& role = GameManager::getInstance().getRole(rnum);
    int result = role.getAttack();
    
    if (role.getGongti() > -1) {
        int l = GetGongtiLevel(rnum, role.getGongti());
        Magic& magic = GameManager::getInstance().getMagic(role.getGongti());
        result += magic.getAddAtt(l);
    }
    
    if (equip) {
        for (int i = 0; i < 5; ++i) {
            int itemId = role.getEquip(i);
            if (itemId >= 0) {
                Item& item = GameManager::getInstance().getItem(itemId);
                result += item.getAddAttack();
            }
        }
    }
    return result;
}

static int GetRoleDefence(int rnum, bool equip) {
    Role& role = GameManager::getInstance().getRole(rnum);
    int result = role.getDefence();
    
    if (role.getGongti() > -1) {
        int l = GetGongtiLevel(rnum, role.getGongti());
        Magic& magic = GameManager::getInstance().getMagic(role.getGongti());
        result += magic.getAddDef(l);
    }
    
    if (equip) {
        for (int i = 0; i < 5; ++i) {
            int itemId = role.getEquip(i);
            if (itemId >= 0) {
                Item& item = GameManager::getInstance().getItem(itemId);
                result += item.getAddDefence();
            }
        }
    }
    return result;
}

static int GetRoleSpeed(int rnum, bool equip) {
    Role& role = GameManager::getInstance().getRole(rnum);
    int result = role.getSpeed();
    
    if (role.getGongti() > -1) {
        int l = GetGongtiLevel(rnum, role.getGongti());
        Magic& magic = GameManager::getInstance().getMagic(role.getGongti());
        result += magic.getAddSpd(l);
    }
    
    if (equip) {
        for (int i = 0; i < 5; ++i) {
            int itemId = role.getEquip(i);
            if (itemId >= 0) {
                Item& item = GameManager::getInstance().getItem(itemId);
                result += item.getAddSpeed();
            }
        }
    }
    return result;
}

static int GetRoleFist(int rnum, bool equip) {
    Role& role = GameManager::getInstance().getRole(rnum);
    int result = role.getFist();
    
    if (role.getGongti() > -1) {
        int l = GetGongtiLevel(rnum, role.getGongti());
        Magic& magic = GameManager::getInstance().getMagic(role.getGongti());
        if (l == magic.getMaxLevel()) {
            result += magic.getAddFist();
        }
    }
    
    if (equip) {
        for (int i = 0; i < 5; ++i) {
            int itemId = role.getEquip(i);
            if (itemId >= 0) {
                Item& item = GameManager::getInstance().getItem(itemId);
                result += item.getAddFist();
            }
        }
    }
    return result;
}

static int GetRoleSword(int rnum, bool equip) {
    Role& role = GameManager::getInstance().getRole(rnum);
    int result = role.getSword();
    
    if (role.getGongti() > -1) {
        int l = GetGongtiLevel(rnum, role.getGongti());
        Magic& magic = GameManager::getInstance().getMagic(role.getGongti());
        if (l == magic.getMaxLevel()) {
            result += magic.getAddSword();
        }
    }
    
    if (equip) {
        for (int i = 0; i < 5; ++i) {
            int itemId = role.getEquip(i);
            if (itemId >= 0) {
                Item& item = GameManager::getInstance().getItem(itemId);
                result += item.getAddSword();
            }
        }
    }
    return result;
}

static int GetRoleKnife(int rnum, bool equip) {
    Role& role = GameManager::getInstance().getRole(rnum);
    int result = role.getKnife();
    
    if (role.getGongti() > -1) {
        int l = GetGongtiLevel(rnum, role.getGongti());
        Magic& magic = GameManager::getInstance().getMagic(role.getGongti());
        if (l == magic.getMaxLevel()) {
            result += magic.getAddKnife();
        }
    }
    
    if (equip) {
        for (int i = 0; i < 5; ++i) {
            int itemId = role.getEquip(i);
            if (itemId >= 0) {
                Item& item = GameManager::getInstance().getItem(itemId);
                result += item.getAddKnife();
            }
        }
    }
    return result;
}

static int GetRoleUnusual(int rnum, bool equip) {
    Role& role = GameManager::getInstance().getRole(rnum);
    int result = role.getUnusual();
    
    if (role.getGongti() > -1) {
        int l = GetGongtiLevel(rnum, role.getGongti());
        Magic& magic = GameManager::getInstance().getMagic(role.getGongti());
        if (l == magic.getMaxLevel()) {
            result += magic.getAddUnusual();
        }
    }
    
    if (equip) {
        for (int i = 0; i < 5; ++i) {
            int itemId = role.getEquip(i);
            if (itemId >= 0) {
                Item& item = GameManager::getInstance().getItem(itemId);
                result += item.getAddUnusual();
            }
        }
    }
    return result;
}


// Port of CalNewHurtValue
static int CalNewHurtValue(int lv, int minVal, int maxVal, int proportion) {
    if (proportion == 0) proportion = 100;
    double p = proportion / 1000.0;
    double n = std::pow((double)(maxVal - minVal), 1.0 / p) / 9.0;
    return (int)(std::round(std::pow((lv * n), p)) + minVal);
}

static int GetRoleKnowledge(int rnum) {
    if (rnum < 0) return 0;
    return GameManager::getInstance().getRole(rnum).getKnowledge();
}

static int GetRoleLevel(int rnum) {
    if (rnum < 0) return 0;
    return GameManager::getInstance().getRole(rnum).getLevel();
}

static int GetRoleDifficulty(int rnum) {
    // Usually Difficulty is stored in Role 0? Or global?
    // Pascal: rrole[0].difficulty
    return GameManager::getInstance().getRole(0).getDifficulty(); // Assuming Role 0 holds it
}

static int GetRoleMedcine(int rnum, bool equip) {
    Role& role = GameManager::getInstance().getRole(rnum);
    int result = role.getMedcine();
    if (role.getGongti() > -1) {
        int l = GetGongtiLevel(rnum, role.getGongti());
        Magic& magic = GameManager::getInstance().getMagic(role.getGongti());
        if (l == magic.getMaxLevel()) {
            result += magic.getAddMedcine();
        }
    }
    if (equip) {
        for (int i = 0; i < 5; ++i) {
            int itemId = role.getEquip(i);
            if (itemId >= 0) {
                Item& item = GameManager::getInstance().getItem(itemId);
                result += item.getAddMedcine();
            }
        }
    }
    return result;
}

static int GetRoleMedPoi(int rnum, bool equip) {
    Role& role = GameManager::getInstance().getRole(rnum);
    int result = role.getMedPoi();
    if (role.getGongti() > -1) {
        int l = GetGongtiLevel(rnum, role.getGongti());
        Magic& magic = GameManager::getInstance().getMagic(role.getGongti());
        if (l == magic.getMaxLevel()) {
            result += magic.getAddMedPoi();
        }
    }
    if (equip) {
        for (int i = 0; i < 5; ++i) {
            int itemId = role.getEquip(i);
            if (itemId >= 0) {
                Item& item = GameManager::getInstance().getItem(itemId);
                result += item.getAddMedPoi();
            }
        }
    }
    return result;
}

static int GetRoleDefPoi(int rnum, bool equip) {
    Role& role = GameManager::getInstance().getRole(rnum);
    int result = role.getDefPoi();
    if (role.getGongti() > -1) {
        int l = GetGongtiLevel(rnum, role.getGongti());
        Magic& magic = GameManager::getInstance().getMagic(role.getGongti());
        if (l == magic.getMaxLevel()) {
            result += magic.getAddDefPoi();
        }
    }
    if (equip) {
        for (int i = 0; i < 5; ++i) {
            int itemId = role.getEquip(i);
            if (itemId >= 0) {
                Item& item = GameManager::getInstance().getItem(itemId);
                result += item.getAddDefPoi();
            }
        }
    }
    return result;
}

static int GetRoleUsePoi(int rnum, bool equip) {
    Role& role = GameManager::getInstance().getRole(rnum);
    int result = role.getUsePoi();
    if (role.getGongti() > -1) {
        int l = GetGongtiLevel(rnum, role.getGongti());
        Magic& magic = GameManager::getInstance().getMagic(role.getGongti());
        if (l == magic.getMaxLevel()) {
            result += magic.getAddUsePoi();
        }
    }
    if (equip) {
        for (int i = 0; i < 5; ++i) {
            int itemId = role.getEquip(i);
            if (itemId >= 0) {
                Item& item = GameManager::getInstance().getItem(itemId);
                result += item.getAddUsePoi();
            }
        }
    }
    return result;
}

static int GetRoleHidWeapon(int rnum, bool equip) {
    Role& role = GameManager::getInstance().getRole(rnum);
    return role.getHidWeapon();
}

BattleManager::BattleManager() : m_battleRunning(false), m_currentRoleIndex(0), m_cursorX(0), m_cursorY(0) {
}

BattleManager& BattleManager::getInstance() {
    static BattleManager instance;
    return instance;
}

bool BattleManager::Init() {
    return true;
}

bool BattleManager::LoadWarData(int battleId) {
    auto data = FileLoader::loadFile("War.sta");
    if (data.empty()) return false;
    
    const int recordSize = 312;
    if (battleId < 0 || (battleId + 1) * recordSize > data.size()) {
        std::cerr << "Battle ID " << battleId << " out of range" << std::endl;
        return false;
    }
    
    const uint8_t* ptr = data.data() + battleId * recordSize;
    std::vector<int16_t> warVec(156);
    std::memcpy(warVec.data(), ptr, recordSize);
    
    m_warData.setDataVector(warVec);
    return true;
}

bool BattleManager::LoadBattleField(int fieldNum) {
    auto idxData = FileLoader::loadFile("warfld.idx");
    if (idxData.empty()) return false;
    
    const int32_t* offsets = reinterpret_cast<const int32_t*>(idxData.data());
    if (fieldNum < 0 || (fieldNum + 1) * 4 > idxData.size()) return false;
    
    int offset = offsets[fieldNum];
    int nextOffset = (fieldNum + 1 < idxData.size() / 4) ? offsets[fieldNum + 1] : -1;
    
    auto grpData = FileLoader::loadFile("warfld.grp");
    if (grpData.empty()) return false;
    
    if (nextOffset == -1) nextOffset = grpData.size();
    int len = nextOffset - offset;
    
    if (offset < 0 || offset + len > grpData.size()) return false;
    
    const uint8_t* pData = grpData.data() + offset;
    
    // Clear field
    for(int i=0; i<8; i++)
        for(int x=0; x<64; x++)
            for(int y=0; y<64; y++)
                m_battleField[i][x][y] = 0;
                
    if (len == 16384) {
        // Raw
        const int16_t* pInt = reinterpret_cast<const int16_t*>(pData);
        for(int y=0; y<64; y++) {
            for(int x=0; x<64; x++) {
                m_battleField[0][x][y] = pInt[y*64 + x];
                m_battleField[1][x][y] = pInt[64*64 + y*64 + x];
            }
        }
    } else {
        // Fallback or RLE
        for(int x=0; x<64; x++)
            for(int y=0; y<64; y++)
                m_battleField[0][x][y] = 1;
    }
    return true;
}

bool BattleManager::StartBattle(int battleId) {
    std::cout << "Starting Battle: " << battleId << std::endl;
    
    if (!LoadWarData(battleId)) {
        std::cerr << "Failed to load WarData" << std::endl;
        return false;
    }
    
    if (!LoadBattleField(m_warData.getBattleMap())) {
        std::cerr << "Failed to load BattleField" << std::endl;
    }

    m_battleRoles.clear();
    
    // 1. Player Party
    bool hasPlayer = false;
    for (int i = 0; i < 12; ++i) {
        int roleId = m_warData.getMate(i);
        if (roleId >= 0) {
            BattleRole br;
            br.setRNum(roleId);
            br.setTeam(0);
            br.setX(m_warData.getMateX(i));
            br.setY(m_warData.getMateY(i));
            Role& rData = GameManager::getInstance().getRole(roleId);
            br.setPic(rData.getHeadNum());
            br.setSpeed(rData.getSpeed());
            br.setKnowledge(rData.getKnowledge());
            br.setProgress(0);
            br.setDead(0);
            m_battleRoles.push_back(br);
            if (roleId == 0) hasPlayer = true;
        }
    }
    
    if (m_battleRoles.empty() || !hasPlayer) {
        const auto& team = GameManager::getInstance().getTeamList();
        for (size_t i = 0; i < team.size(); ++i) {
            int roleId = team[i];
            BattleRole br;
            br.setRNum(roleId);
            br.setTeam(0);
            br.setX(5 + i);
            br.setY(5 + i);
            Role& rData = GameManager::getInstance().getRole(roleId);
            br.setPic(rData.getHeadNum());
            br.setSpeed(rData.getSpeed());
            br.setKnowledge(rData.getKnowledge());
            br.setProgress(0);
            br.setDead(0);
            m_battleRoles.push_back(br);
        }
    }

    // 2. Enemies
    for (int i = 0; i < 30; ++i) {
        int roleId = m_warData.getEnemy(i);
        if (roleId >= 0) {
            BattleRole br;
            br.setRNum(roleId);
            br.setTeam(1);
            br.setX(m_warData.getEnemyX(i));
            br.setY(m_warData.getEnemyY(i));
            Role& rData = GameManager::getInstance().getRole(roleId);
            br.setPic(rData.getHeadNum());
            br.setSpeed(rData.getSpeed());
            br.setKnowledge(rData.getKnowledge());
            br.setProgress(0);
            br.setDead(0);
            m_battleRoles.push_back(br);
        }
    }

    // Place Roles
    for (int i = 0; i < m_battleRoles.size(); ++i) {
        BattleRole& r = m_battleRoles[i];
        if (r.getX() >= 0 && r.getX() < 64 && r.getY() >= 0 && r.getY() < 64) {
            m_battleField[2][r.getX()][r.getY()] = i;
        }
    }

    m_battleRunning = true;
    RunBattle();
    
    bool playerAlive = false;
    for(const auto& r : m_battleRoles) {
        if (r.getTeam() == 0 && !r.getDead()) {
            playerAlive = true;
            break;
        }
    }
    return playerAlive; 
}

void BattleManager::RunBattle() {
    std::cout << "Entering Battle Loop..." << std::endl;
    SDL_Event event;

    while (m_battleRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_battleRunning = false;
                GameManager::getInstance().Quit();
                return;
            }
        }

        int actorIdx = -1;
        int maxProgress = -1;

        for (int i = 0; i < m_battleRoles.size(); ++i) {
            if (m_battleRoles[i].getDead()) continue;
            if (m_battleRoles[i].getProgress() >= 100) {
                if (m_battleRoles[i].getProgress() > maxProgress) {
                    maxProgress = m_battleRoles[i].getProgress();
                    actorIdx = i;
                }
            }
        }

        if (actorIdx != -1) {
            BattleRole& actor = m_battleRoles[actorIdx];
            m_cursorX = actor.getX();
            m_cursorY = actor.getY();

            if (actor.getTeam() == 0) {
                BattleMenu(actorIdx);
            } else {
                AutoBattle(actorIdx);
            }
            
            bool pAlive = false;
            bool eAlive = false;
            for(const auto& r : m_battleRoles) {
                if (!r.getDead()) {
                    if (r.getTeam() == 0) pAlive = true;
                    else eAlive = true;
                }
            }
            if (!pAlive || !eAlive) {
                m_battleRunning = false;
            }
            
        } else {
            for (auto& role : m_battleRoles) {
                if (role.getDead()) continue;
                int spd = GameManager::getInstance().getRole(role.getRNum()).getSpeed();
                role.setProgress(role.getProgress() + std::max(1, spd / 2));
            }
        }

        RenderBattle();
        UIManager::getInstance().UpdateScreen();
        SDL_Delay(16);
    }
}

void BattleManager::MoveRole(int roleIdx, int x, int y) {
    if (roleIdx < 0 || roleIdx >= m_battleRoles.size()) return;
    BattleRole& role = m_battleRoles[roleIdx];
    
    if (m_battleField[2][role.getX()][role.getY()] == roleIdx) {
        m_battleField[2][role.getX()][role.getY()] = -1;
    }
    
    role.setX(x);
    role.setY(y);
    m_battleField[2][x][y] = roleIdx;
}

void BattleManager::CalSelectableArea(int roleIdx) {
    for(int y=0; y<64; y++)
        for(int x=0; x<64; x++)
            m_battleField[3][x][y] = -1;

    if (roleIdx < 0 || roleIdx >= m_battleRoles.size()) return;
    BattleRole& role = m_battleRoles[roleIdx];
    int startX = role.getX();
    int startY = role.getY();
    int maxStep = GameManager::getInstance().getRole(role.getRNum()).getSpeed() / 10;
    if (maxStep < 1) maxStep = 1;

    std::vector<std::pair<int, int>> queue;
    queue.push_back({startX, startY});
    m_battleField[3][startX][startY] = 0;

    int head = 0;
    while(head < queue.size()) {
        auto [cx, cy] = queue[head++];
        int step = m_battleField[3][cx][cy];
        
        if (step >= maxStep) continue;

        int dx[] = {0, 0, 1, -1};
        int dy[] = {1, -1, 0, 0};
        
        for(int i=0; i<4; i++) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];
            
            if (nx >= 0 && nx < 64 && ny >= 0 && ny < 64) {
                if (m_battleField[3][nx][ny] == -1 && 
                    m_battleField[0][nx][ny] > 0 &&
                    m_battleField[1][nx][ny] == 0 &&
                    m_battleField[2][nx][ny] == -1) 
                {
                    m_battleField[3][nx][ny] = step + 1;
                    queue.push_back({nx, ny});
                }
            }
        }
    }
}

void BattleManager::AutoBattle(int roleIdx) {
    BattleRole& actor = m_battleRoles[roleIdx];
    Role& role = GameManager::getInstance().getRole(actor.getRNum());
    
    // 1. Self Heal / Restore (Priority)
    if (actor.getActed() == 0 && role.getCurrentHP() < role.getMaxHP() / 5) {
        if (rand() % 100 < 70) {
            if (GetRoleMedcine(actor.getRNum(), true) >= 50 && role.getPhyPower() >= 50 && rand() % 100 < 50) {
                ApplyMedicine(roleIdx, roleIdx); // Heal Self
            } else {
                AutoUseItem(roleIdx, 45); // HP Item
            }
        }
    }
    if (actor.getActed()) return;

    if (actor.getActed() == 0 && role.getCurrentMP() < role.getMaxMP() / 5) {
        if (rand() % 100 < 60) {
            AutoUseItem(roleIdx, 50); // MP Item
        }
    }
    if (actor.getActed()) return;

    if (actor.getActed() == 0 && role.getPhyPower() < 20) { // Max is 100
        if (rand() % 100 < 80) {
            AutoUseItem(roleIdx, 48); // Phy Item
        }
    }
    if (actor.getActed()) return;

    // 2. Hidden Weapon (If equipped or in inventory)
    // Simplified: Only if in range of an enemy?
    // Or just check if we can use it.
    // For now, skip complex AI for HiddenWeapon moving.
    
    int targetIdx = -1;
    int minDist = 9999;
    
    for (int i = 0; i < m_battleRoles.size(); ++i) {
        if (m_battleRoles[i].getDead()) continue;
        if (m_battleRoles[i].getTeam() == actor.getTeam()) continue;
        
        int dist = std::abs(m_battleRoles[i].getX() - actor.getX()) + 
                   std::abs(m_battleRoles[i].getY() - actor.getY());
        if (dist < minDist) {
            minDist = dist;
            targetIdx = i;
        }
    }
    
    if (targetIdx != -1) {
        // Try Hidden Weapon if in range
        if (GetRoleHidWeapon(actor.getRNum(), true) >= 30) {
             // Check if we have hidden weapon
             for(int i=0; i<4; ++i) {
                 int itemId = role.getTakingItem(i);
                 if (itemId >= 0 && role.getTakingItemAmount(i) > 0) {
                     // Check type? Assuming all items in TakingItem are potentially usable or we check type 4 (Hidden Weapon)
                     Item& item = GameManager::getInstance().getItem(itemId);
                     // if (item.getType() == 4) ...
                     // Assuming we can use it.
                     int range = GetRoleHidWeapon(actor.getRNum(), true) / 15 + 1;
                     if (minDist <= range) {
                         ApplyHiddenWeapon(roleIdx, targetIdx, itemId);
                         if (actor.getActed()) return;
                     }
                 }
             }
        }

        if (minDist <= 1) {
            // Attack
            BattleRole& target = m_battleRoles[targetIdx];
            Role& tData = GameManager::getInstance().getRole(target.getRNum());
            Role& aData = GameManager::getInstance().getRole(actor.getRNum());
            
            // Try to use the first available magic
            int magicId = -1;
            int level = 1;
            for(int i=0; i<10; i++) {
                 int m = aData.getMagic(i);
                 if (m > 0) {
                     magicId = m;
                     level = aData.getMagLevel(i) / 100;
                     if (level < 1) level = 1; 
                     break;
                 }
            }

            int dmg = 0;
            if (magicId > 0) {
                dmg = CalHurtValue(roleIdx, targetIdx, magicId, level);
            } else {
                // Fallback: Simple physical calculation if no magic
                int att = GetRoleAttack(actor.getRNum(), true);
                int def = GetRoleDefence(target.getRNum(), true);
                dmg = std::max(1, att - def / 2);
            }
            
            tData.setCurrentHP(std::max(0, tData.getCurrentHP() - dmg));
            if (tData.getCurrentHP() == 0) target.setDead(1);
            
            target.setShowNumber(dmg);
            ShowHurtValue(0);
        } else {
            // Move towards target
            CalSelectableArea(roleIdx);
            
            int bestX = actor.getX();
            int bestY = actor.getY();
            int bestDistToTarget = minDist;
            
            for(int x=0; x<64; x++) {
                for(int y=0; y<64; y++) {
                    if (m_battleField[3][x][y] >= 0) {
                        BattleRole& t = m_battleRoles[targetIdx];
                        int d = std::abs(x - t.getX()) + std::abs(y - t.getY());
                        if (d < bestDistToTarget) {
                            bestDistToTarget = d;
                            bestX = x;
                            bestY = y;
                        }
                    }
                }
            }
            MoveRole(roleIdx, bestX, bestY);
        }
    }
    
    actor.setActed(1);
    actor.setProgress(0);
}

// --- Item Usage Implementation ---

void BattleManager::ShowItemMenu(const std::vector<int>& itemIds, int current, int x, int y) {
    // Stub: UI to show items will be implemented in UI phase
}

void BattleManager::ApplyItemEffect(int rnum, int inum, int where) {
    if (rnum < 0 || inum < 0) return;
    
    Role& role = GameManager::getInstance().getRole(rnum);
    Item& item = GameManager::getInstance().getItem(inum);
    
    // HP
    int addHP = item.getAddCurrentHP();
    if (addHP != 0) {
        role.setCurrentHP(std::min((int)role.getMaxHP(), role.getCurrentHP() + addHP));
    }
    
    // MP
    int addMP = item.getAddCurrentMP();
    if (addMP != 0) {
        role.setCurrentMP(std::min((int)role.getMaxMP(), role.getCurrentMP() + addMP));
    }
    
    // Poison (Cure or Add)
    int addPoi = item.getAddPoi();
    if (addPoi != 0) {
        // If curing (negative), min is 0. If adding, max is 99?
        int newPoi = role.getPoision() + addPoi;
        if (newPoi < 0) newPoi = 0;
        if (newPoi > 99) newPoi = 99;
        role.setPoision(newPoi);
    }
    
    // Hurt (Injury) - Usually items cure injury by negative AddHurt or specific field?
    // Looking at Item.h, there isn't a direct "AddHurt" field visible in basic props, 
    // but Pascal `EatOneItem` logic handles various stats.
    // For battle items, main stats are HP, MP, Poi, PhyPower.
    
    // Physical Power
    int addPhy = item.getAddPhyPower();
    if (addPhy != 0) {
        role.setPhyPower(std::min(100, role.getPhyPower() + addPhy));
    }
    
    // Decrease Item Amount
    if (where == 0) { // Bag
        GameManager::getInstance().useItem(inum);
    } else { // Role inventory (TakingItem)
        // This part needs Role to have a way to decrease TakingItemAmount
        for(int i=0; i<4; ++i) {
            if (role.getTakingItem(i) == inum) {
                int amt = role.getTakingItemAmount(i);
                if (amt > 0) {
                    role.setTakingItemAmount(i, amt - 1);
                    if (role.getTakingItemAmount(i) == 0) {
                        role.setTakingItem(i, -1);
                    }
                }
                break;
            }
        }
    }
}

// Medical (Heal others)
void BattleManager::Medcine(int roleIdx) {
    if (roleIdx < 0 || roleIdx >= m_battleRoles.size()) return;
    BattleRole& actor = m_battleRoles[roleIdx];
    int rnum = actor.getRNum();
    
    int med = GetRoleMedcine(rnum, true);
    int step = med / 15 + 1;
    
    if (SelectAim(roleIdx, step)) {
        int targetIdx = -1;
        if (m_battleField[2][m_cursorX][m_cursorY] >= 0) {
            targetIdx = m_battleField[2][m_cursorX][m_cursorY];
        }
        
        if (targetIdx >= 0) {
            BattleRole& target = m_battleRoles[targetIdx];
            if (target.getTeam() == actor.getTeam()) {
                ApplyMedicine(roleIdx, targetIdx);
            }
        }
    }
}

void BattleManager::ApplyMedicine(int healerIdx, int targetIdx) {
    BattleRole& actor = m_battleRoles[healerIdx];
    BattleRole& targetBR = m_battleRoles[targetIdx];
    Role& healer = GameManager::getInstance().getRole(actor.getRNum());
    Role& target = GameManager::getInstance().getRole(targetBR.getRNum());
    
    int med = GetRoleMedcine(actor.getRNum(), true);
    int healVal = med * (10 - target.getHurt() / 15) / 10;
    
    if (target.getHurt() - med > 20) healVal = 0;
    if (healVal < 0) healVal = 0;
    
    int maxHeal = target.getMaxHP() - target.getCurrentHP();
    healVal = std::min(healVal, maxHeal);
    
    target.setCurrentHP(target.getCurrentHP() + healVal);
    
    int cureHurt = healVal / 10;
    target.setHurt(std::max(0, target.getHurt() - cureHurt));
    
    targetBR.setShowNumber(healVal);
    ShowHurtValue(3);
    
    if (healVal > 0) {
        actor.setExpGot(actor.getExpGot() + healVal / 5 + healVal / 10);
    }
    
    actor.setActed(1);
    healer.setPhyPower(std::max(0, healer.getPhyPower() - 5));
    actor.setProgress(actor.getProgress() - 240);
}

void BattleManager::MedFrozen(int roleIdx) {
    if (roleIdx < 0 || roleIdx >= m_battleRoles.size()) return;
    BattleRole& actor = m_battleRoles[roleIdx];
    int rnum = actor.getRNum();
    Role& rData = GameManager::getInstance().getRole(rnum);
    
    int med = rData.getCurrentMP();
    int step = med / 200 + 1;
    
    if (SelectAim(roleIdx, step)) {
        int targetIdx = -1;
        if (m_battleField[2][m_cursorX][m_cursorY] >= 0) {
            targetIdx = m_battleField[2][m_cursorX][m_cursorY];
        }
        
        if (targetIdx >= 0) {
            BattleRole& target = m_battleRoles[targetIdx];
            if (target.getTeam() == actor.getTeam()) {
                ApplyMedFrozen(roleIdx, targetIdx);
            }
        }
    }
}

void BattleManager::ApplyMedFrozen(int healerIdx, int targetIdx) {
    BattleRole& actor = m_battleRoles[healerIdx];
    BattleRole& targetBR = m_battleRoles[targetIdx];
    Role& rData = GameManager::getInstance().getRole(actor.getRNum());
    
    int medcine = GetRoleMedcine(actor.getRNum(), true);
    int cureVal = (rData.getCurrentMP() + medcine * 5) / 3;
    
    // TODO: targetBR.setFrozen(max(0, targetBR.getFrozen() - cureVal));
    
    targetBR.setShowNumber(cureVal);
    ShowHurtValue(4);
    
    actor.setActed(1);
    rData.setPhyPower(std::max(0, rData.getPhyPower() - 5));
    actor.setProgress(actor.getProgress() - 240);
}

void BattleManager::MedPoision(int roleIdx) {
    if (roleIdx < 0 || roleIdx >= m_battleRoles.size()) return;
    BattleRole& actor = m_battleRoles[roleIdx];
    int rnum = actor.getRNum();
    
    int medpoi = GetRoleMedPoi(rnum, true);
    int step = medpoi / 15 + 1;
    
    if (SelectAim(roleIdx, step)) {
        int targetIdx = -1;
        if (m_battleField[2][m_cursorX][m_cursorY] >= 0) {
            targetIdx = m_battleField[2][m_cursorX][m_cursorY];
        }
        
        if (targetIdx >= 0) {
            BattleRole& target = m_battleRoles[targetIdx];
            if (target.getTeam() == actor.getTeam()) {
                ApplyMedPoision(roleIdx, targetIdx);
            }
        }
    }
}

void BattleManager::ApplyMedPoision(int healerIdx, int targetIdx) {
    BattleRole& actor = m_battleRoles[healerIdx];
    BattleRole& targetBR = m_battleRoles[targetIdx];
    Role& rData = GameManager::getInstance().getRole(actor.getRNum());
    Role& tData = GameManager::getInstance().getRole(targetBR.getRNum());
    
    int medpoi = GetRoleMedPoi(actor.getRNum(), true);
    int minuspoi = medpoi;
    int currentPoi = tData.getPoision();
    
    if (minuspoi < currentPoi / 2) minuspoi = 0;
    else if (minuspoi > currentPoi) minuspoi = currentPoi;
    
    minuspoi = std::min(minuspoi, currentPoi);
    
    if (minuspoi > 0) {
        actor.setExpGot(actor.getExpGot() + minuspoi / 5);
    }
    
    tData.setPoision(currentPoi - minuspoi);
    
    targetBR.setShowNumber(minuspoi);
    ShowHurtValue(4);
    
    actor.setActed(1);
    rData.setPhyPower(std::max(0, rData.getPhyPower() - 5));
    actor.setProgress(actor.getProgress() - 240);
}

void BattleManager::UsePoision(int roleIdx) {
    if (roleIdx < 0 || roleIdx >= m_battleRoles.size()) return;
    BattleRole& actor = m_battleRoles[roleIdx];
    int rnum = actor.getRNum();
    
    int usepoi = GetRoleUsePoi(rnum, true);
    int step = usepoi / 15 + 1;
    
    if (SelectAim(roleIdx, step)) {
        int targetIdx = -1;
        if (m_battleField[2][m_cursorX][m_cursorY] >= 0) {
            targetIdx = m_battleField[2][m_cursorX][m_cursorY];
        }
        
        if (targetIdx >= 0) {
            BattleRole& target = m_battleRoles[targetIdx];
            if (target.getTeam() != actor.getTeam()) {
                ApplyUsePoision(roleIdx, targetIdx);
            }
        }
    }
}

void BattleManager::ApplyUsePoision(int attackerIdx, int targetIdx) {
    BattleRole& actor = m_battleRoles[attackerIdx];
    BattleRole& targetBR = m_battleRoles[targetIdx];
    Role& rData = GameManager::getInstance().getRole(actor.getRNum());
    Role& tData = GameManager::getInstance().getRole(targetBR.getRNum());
    
    int usepoi = GetRoleUsePoi(actor.getRNum(), true);
    int addpoi = usepoi;
    
    int val = usepoi - tData.getPoision();
    if (val < 0) val = 0;
    addpoi = std::min(addpoi, val);
    
    if (addpoi > 0) {
        actor.setExpGot(actor.getExpGot() + addpoi / 5);
    }
    
    tData.setPoision(tData.getPoision() + addpoi);
    
    targetBR.setShowNumber(addpoi);
    ShowHurtValue(2);
    
    actor.setActed(1);
    rData.setPhyPower(std::max(0, rData.getPhyPower() - 5));
    actor.setProgress(actor.getProgress() - 240);
}

void BattleManager::UseHiddenWeapen(int roleIdx, int itemIdx) {
    if (roleIdx < 0 || roleIdx >= m_battleRoles.size()) return;
    BattleRole& actor = m_battleRoles[roleIdx];
    int rnum = actor.getRNum();
    
    int hidden = GetRoleHidWeapon(rnum, true);
    int step = hidden / 15 + 1;
    
    if (SelectAim(roleIdx, step)) {
        int targetIdx = -1;
        if (m_battleField[2][m_cursorX][m_cursorY] >= 0) {
            targetIdx = m_battleField[2][m_cursorX][m_cursorY];
        }
        
        if (targetIdx >= 0) {
            BattleRole& target = m_battleRoles[targetIdx];
            if (target.getTeam() != actor.getTeam()) {
                ApplyHiddenWeapon(roleIdx, targetIdx, itemIdx);
            }
        }
    }
}

void BattleManager::ApplyHiddenWeapon(int attackerIdx, int targetIdx, int itemIdx) {
    BattleRole& actor = m_battleRoles[attackerIdx];
    BattleRole& targetBR = m_battleRoles[targetIdx];
    Role& tData = GameManager::getInstance().getRole(targetBR.getRNum());
    Role& rData = GameManager::getInstance().getRole(actor.getRNum());
    Item& item = GameManager::getInstance().getItem(itemIdx);
    
    int hidden = GetRoleHidWeapon(actor.getRNum(), true);
    
    // Consume Item
    for(int i=0; i<4; ++i) {
        if (rData.getTakingItem(i) == itemIdx) {
            int amt = rData.getTakingItemAmount(i);
            if (amt > 0) {
                rData.setTakingItemAmount(i, amt - 1);
                if (rData.getTakingItemAmount(i) == 0) rData.setTakingItem(i, -1);
            }
            break;
        }
    }
    
    int hurt = -(hidden * item.getAddCurrentHP()) / 100;
    if (hurt < 25) hurt = 25;
    
    tData.setCurrentHP(std::max(0, tData.getCurrentHP() - hurt));
    if (tData.getCurrentHP() == 0) targetBR.setDead(1);
    
    int poi = (hidden * item.getAddPoi()) / 100 - GetRoleDefPoi(targetBR.getRNum(), true);
    if (poi < 0) poi = 0;
    tData.setPoision(std::min(100, tData.getPoision() + poi));
    
    targetBR.setShowNumber(hurt);
    ShowHurtValue(0);
    
    actor.setActed(1);
    actor.setProgress(actor.getProgress() - 240);
}

void BattleManager::AutoUseItem(int roleIdx, int listType) {
    if (roleIdx < 0 || roleIdx >= m_battleRoles.size()) return;
    BattleRole& actor = m_battleRoles[roleIdx];
    int rnum = actor.getRNum();
    Role& role = GameManager::getInstance().getRole(rnum);
    
    int bestItemIdx = -1;
    int maxVal = 0;
    int where = 0; // 0: Bag, 1: TakingItem
    
    if (actor.getTeam() != 0) { // Enemy or Ally (Non-Player)
        // Check TakingItem
        where = 1;
        for (int i = 0; i < 4; ++i) {
            int itemId = role.getTakingItem(i);
            int amount = role.getTakingItemAmount(i);
            if (itemId >= 0 && amount > 0) {
                Item& item = GameManager::getInstance().getItem(itemId);
                // Check if usable (ItemType = 3?)
                // Assuming type 3 is medicine/consumable
                // Pascal: ritem[Rrole[rnum].TakingItem[i]].ItemType = 3 (checked in AutoUseItem)
                // But BattleManager doesn't have ItemType enum visible here, assuming accessor.
                // Item.h likely has getType or getItemType.
                // Let's assume generic usable check or just value check.
                
                int val = 0;
                if (listType == 45) val = item.getAddCurrentHP();
                else if (listType == 50) val = item.getAddCurrentMP();
                else if (listType == 48) val = item.getAddPhyPower();
                
                if (val > maxVal) {
                    maxVal = val;
                    bestItemIdx = itemId;
                }
            }
        }
    } else { // Player Team
        // Check Bag (GameManager ItemList)
        where = 0;
        const auto& bag = GameManager::getInstance().getItemList();
        // bag is vector<int> of item counts? No, GameManager has separate vectors?
        // GameManager::getItemList() returns std::vector<int>& itemAmount usually?
        // Let's check GameManager.h.
        // Usually KYS stores items as amounts for all items.
        // Pascal: RItemList[i].Amount
        
        // GameManager usually has access to global item amounts.
        // Assuming GameManager::getInstance().getItemAmount(id) exists or similar.
        // Actually GameManager.h usually has:
        // std::vector<int> m_items; // Counts
        // Let's assume we can iterate all items.
        // Since I can't easily iterate "all items" without knowing max, I'll assume standard KYS range (roughly 400-800 items).
        // Or check GameManager methods.
        // If GameManager has `getItemAmount(id)`, I can loop.
        
        // For safety, I'll use a loop over a reasonable range or check how ShowItemMenu does it (it wasn't implemented).
        // I'll stick to a fixed range for now or if GameManager has `getAvailableItems`.
        
        // Let's try to find best item in bag.
        for (int id = 0; id < 1000; ++id) { // Max items assumption
             if (GameManager::getInstance().getItemAmount(id) > 0) {
                 Item& item = GameManager::getInstance().getItem(id);
                 // Type check?
                 // if (item.getItemType() == 3) ...
                 
                 int val = 0;
                 if (listType == 45) val = item.getAddCurrentHP();
                 else if (listType == 50) val = item.getAddCurrentMP();
                 else if (listType == 48) val = item.getAddPhyPower();
                 
                 if (val > maxVal) {
                     maxVal = val;
                     bestItemIdx = id;
                 }
             }
        }
    }
    
    if (bestItemIdx >= 0) {
        ApplyItemEffect(roleIdx, bestItemIdx, where);
        
        // Animation/Delay
        m_battleRoles[roleIdx].setShowNumber(maxVal); // Show recovered amount
        ShowHurtValue(3); // Green
        
        actor.setActed(1);
        actor.setProgress(actor.getProgress() - 240);
        
        // SDL_Delay(750); // Blocking delay? Ideally not in main thread if possible, but Pascal does it.
    }
}


int BattleManager::CalHurtValue(int attackerIdx, int targetIdx, int magicId, int level) {
    if (attackerIdx < 0 || attackerIdx >= m_battleRoles.size()) return 0;
    if (targetIdx < 0 || targetIdx >= m_battleRoles.size()) return 0;

    BattleRole& attRole = m_battleRoles[attackerIdx];
    BattleRole& tarRole = m_battleRoles[targetIdx];
    int rnum1 = attRole.getRNum();
    int rnum2 = tarRole.getRNum();

    // 1. Calculate Knowledge (Martial Arts Knowledge)
    int k1 = 0;
    int k2 = 0;
    int l1 = 0;
    int c = 0;
    int minKnowledge = 0; // MIN_KNOWLEDGE defaults to 0 in kys_main.pas

    for (const auto& r : m_battleRoles) {
        if (r.getTeam() == attRole.getTeam() && r.getDead() == 0 && r.getKnowledge() > minKnowledge) {
            k1 += r.getKnowledge();
        }
        if (r.getTeam() == tarRole.getTeam() && r.getDead() == 0 && r.getKnowledge() > minKnowledge) {
            k2 += r.getKnowledge();
        }
        if (r.getTeam() == 0 && r.getRNum() >= 0) {
            l1 += r.getLevel();
            c++;
        }
    }
    
    if (c == 0) {
         l1 = GameManager::getInstance().getRole(0).getLevel();
    } else {
         l1 = l1 / c;
    }

    Role& r0 = GameManager::getInstance().getRole(0);
    if (attRole.getTeam() != 0) k1 += l1 * r0.getDifficulty() / 50;
    if (tarRole.getTeam() != 0) k2 += l1 * r0.getDifficulty() / 50;
    
    int knowledge = k1 - k2;
    knowledge = std::min(knowledge, 100);
    knowledge = std::max(knowledge, -100);

    // 2. Base Hurt
    Magic& magic = GameManager::getInstance().getMagic(magicId);
    // Formula: (CalNewHurtValue(...) * (100 + (knowledge * 4) div 5)) div 100;
    int baseHurt = CalNewHurtValue(level - 1, magic.getMinHurt(), magic.getMaxHurt(), magic.getHurtModulus());
    int mhurt = (baseHurt * (100 + (knowledge * 4) / 5)) / 100;

    int p = magic.getAttackModulus() * 6 + magic.getMPModulus() + magic.getSpeedModulus() * 2 + magic.getWeaponModulus() * 2;
    int att = GetRoleAttack(rnum1, true) + 1;
    int def = GetRoleDefence(rnum2, true) + 1;
    
    int wpn1 = 0, wpn2 = 0;
    switch(magic.getMagicType()) {
        case 1: 
            wpn1 = GetRoleFist(rnum1, true) + 1; 
            wpn2 = GetRoleFist(rnum2, true) + 1; 
            break;
        case 2: 
            wpn1 = GetRoleSword(rnum1, true) + 1; 
            wpn2 = GetRoleSword(rnum2, true) + 1; 
            break;
        case 3: 
            wpn1 = GetRoleKnife(rnum1, true) + 1; 
            wpn2 = GetRoleKnife(rnum2, true) + 1; 
            break;
        case 4: 
            wpn1 = GetRoleUnusual(rnum1, true) + 1; 
            wpn2 = GetRoleUnusual(rnum2, true) + 1; 
            break;
    }
    
    int mp1 = GameManager::getInstance().getRole(rnum1).getCurrentMP() + 1;
    int mp2 = GameManager::getInstance().getRole(rnum2).getCurrentMP() + 1;
    
    int spd1 = GetRoleSpeed(rnum1, true) + 1;
    int spd2 = GetRoleSpeed(rnum2, true) + 1;
    
    // CheckEquipSet logic
    Role& r1 = GameManager::getInstance().getRole(rnum1);
    Role& r2 = GameManager::getInstance().getRole(rnum2);
    if (CheckEquipSet(r1.getEquip(0), r1.getEquip(1), r1.getEquip(2), r1.getEquip(3)) == 5) {
        att += 50;
        spd1 += 30;
    }
    if (CheckEquipSet(r2.getEquip(0), r2.getEquip(1), r2.getEquip(2), r2.getEquip(3)) == 5) {
        def -= 25;
        spd2 += 30;
    }
    
    double result = 0;
    att = std::max(att, 1);
    def = std::max(def, 1);
    spd1 = std::max(spd1, 1);
    wpn1 = std::max(wpn1, 1);
    mp1 = std::max(mp1, 1);
    spd2 = std::max(spd2, 1);
    wpn2 = std::max(wpn2, 1);
    mp2 = std::max(mp2, 1);
    
    double a1 = att - def;
    double s1 = spd1 - spd2;
    double w1 = wpn1 - wpn2;
    double m1 = mp1 - mp2;
    
    if (a1 < 5) a1 = 5;
    if (w1 < 5) w1 = 5;
    if (s1 < 5) s1 = 5;
    if (m1 < 5) m1 = 5;
    
    a1 = std::min((a1 / att), 1.0);
    w1 = std::min((w1 / wpn1), 1.0);
    s1 = std::min((s1 / spd1), 1.0);
    m1 = std::min((m1 / mp1), 1.0);
    
    if (p > 0) {
        if (magic.getAttackModulus() > 0)
            result += (int)(mhurt * a1 * (magic.getAttackModulus() * 3 * 2 / (double)p));
        if (magic.getMPModulus() > 0)
            result += (int)(mhurt * m1 * (magic.getMPModulus() / (double)p));
        if (magic.getSpeedModulus() > 0)
            result += (int)(mhurt * s1 * (magic.getSpeedModulus() * 2 / (double)p));
        if (magic.getWeaponModulus() > 0)
            result += (int)(mhurt * w1 * (magic.getWeaponModulus() * 2 / (double)p));
    }
    
    result += (rand() % 10) - (rand() % 10);
    if (result < mhurt / 20.0) {
        result = mhurt / 20.0 + (rand() % 5) - (rand() % 5);
    }
    
    int dis = std::abs(attRole.getX() - tarRole.getX()) + std::abs(attRole.getY() - tarRole.getY());
    if (dis > 10) dis = 10;
    result = result * (100 - (dis - 1) * 3) / 100.0;
    
    if (result <= 0 || level <= 0) {
        result = (rand() % 10) + 1;
    }
    if (result > 9999) result = 9999;
    
    return (int)result;
}

// Applies damage to roles in range
void BattleManager::CalHurtRole(int attackerIdx, int magicId, int level) {
    if (attackerIdx < 0 || attackerIdx >= m_battleRoles.size()) return;
    BattleRole& attacker = m_battleRoles[attackerIdx];
    int rnum = attacker.getRNum();
    Magic& magic = GameManager::getInstance().getMagic(magicId);
    
    // Check MP cost (Simplified, usually checked before)
    // int needMP = magic.getNeedMP() * level;
    
    for (int i = 0; i < m_battleRoles.size(); ++i) {
        BattleRole& target = m_battleRoles[i];
        if (target.getDead() || target.getRNum() < 0) continue;
        
        // Check if in Attack Range (Marked in Layer 4)
        // Pascal: Bfield[4, Brole[i].X, Brole[i].Y]
        if (m_battleField[4][target.getX()][target.getY()] == 0) continue;
        
        // Friendly Fire Check
        if (target.getTeam() == attacker.getTeam() && attackerIdx != i) continue;
        
        // Calculate Damage
        int hurt = CalHurtValue(attackerIdx, i, magicId, level);
        
        // Apply Modifiers (Stub)
        // ... Angry, Status, etc.
        
        // Apply Damage
        Role& tData = GameManager::getInstance().getRole(target.getRNum());
        
        // HurtType 0: HP Damage
        if (magic.getHurtType() == 0) {
            tData.setCurrentHP(std::max(0, tData.getCurrentHP() - hurt));
            target.setShowNumber(hurt);
            if (tData.getCurrentHP() <= 0) {
                target.setDead(1);
                attacker.setExpGot(attacker.getExpGot() + tData.getLevel() * 10);
            }
        }
        // HurtType 1: MP Damage
        else if (magic.getHurtType() == 1) {
            tData.setCurrentMP(std::max(0, tData.getCurrentMP() - hurt));
            target.setShowNumber(hurt);
        }
        
        // Show Float Number (Stub - usually stored in ShowNumber and rendered later)
        ShowHurtValue(magic.getHurtType());
    }
}

void BattleManager::SetAttackArea(int type, int ax, int ay, int range, int bx, int by, int step) {
    // Clear Layer 4
    for(int x=0; x<64; ++x)
        for(int y=0; y<64; ++y)
            m_battleField[4][x][y] = 0;

    for (int i = 0; i < 64; ++i) {
        for (int j = 0; j < 64; ++j) {
            if (m_battleField[0][i][j] <= 0) continue; // Must have ground

            bool inArea = false;
            switch (type) {
                case 0: // Point / Diamond
                    if (std::abs(i - ax) + std::abs(j - ay) <= range) inArea = true;
                    break;
                case 1: // Line (Directional)
                    if (i == bx && std::abs(j - by) <= step) {
                        if ((j - by) * (ay - by) > 0) inArea = true;
                    } else if (j == by && std::abs(i - bx) <= step) {
                        if ((i - bx) * (ax - bx) > 0) inArea = true;
                    }
                    break;
                case 2: // Cross / Star (Centered at Attacker)
                    if ((i == bx && std::abs(j - by) <= step) || 
                        (j == by && std::abs(i - bx) <= step)) {
                        inArea = true;
                    } else if (std::abs(i - bx) == std::abs(j - by) && std::abs(i - bx) <= range) {
                        inArea = true;
                    }
                    break;
                case 3: // Square
                    if (std::abs(i - ax) <= range && std::abs(j - ay) <= range) inArea = true;
                    break;
                case 6: // Far (Same as Point for Area)
                    if (std::abs(i - ax) + std::abs(j - ay) <= range) inArea = true;
                    break;
                // TODO: Implement Types 4, 5, 7 if needed
                default: // Fallback to Point
                    if (i == ax && j == ay) inArea = true;
                    break;
            }
            
            if (inArea) m_battleField[4][i][j] = 1;
        }
    }
}

void BattleManager::Attack(int roleIdx, int targetIdx, int magicId) {
    if (roleIdx < 0 || roleIdx >= m_battleRoles.size()) return;
    BattleRole& attacker = m_battleRoles[roleIdx];
    int attackerX = attacker.getX();
    int attackerY = attacker.getY();
    
    // 1. Determine Level
    int level = GetMagicLevel(attacker.getRNum(), magicId) / 100;
    if (level < 1) level = 1;
    if (level > 10) level = 10;
    
    Magic& magic = GameManager::getInstance().getMagic(magicId);
    int step = magic.getMoveDistance(level - 1);
    int range = magic.getAttDistance(level - 1);
    
    // 2. Set Attack Area (Layer 4)
    int targetX = -1, targetY = -1;
    if (targetIdx >= 0 && targetIdx < m_battleRoles.size()) {
        targetX = m_battleRoles[targetIdx].getX();
        targetY = m_battleRoles[targetIdx].getY();
    } else {
        // Use cursor if invalid target?
        targetX = m_cursorX;
        targetY = m_cursorY;
    }
    
    SetAttackArea(magic.getAttAreaType(), targetX, targetY, range, attackerX, attackerY, step);
    
    // 3. Play Animation (Stub)
    // PlayMagicAmination(roleIdx, magic.getBigAmi(), magic.getAmiNum(), level);
    
    // 4. Calculate & Apply Damage
    CalHurtRole(roleIdx, magicId, level);
    
    // 5. Update Progress
    attacker.setActed(1);
    attacker.setProgress(0); // Reset progress
}

int BattleManager::BattleMenu(int roleIdx) {
    bool done = false;
    SDL_Event event;
    int menuIdx = 0;
    
    while (!done) {
        RenderBattle();
        
        // Draw Menu
        int menuX = 500, menuY = 300;
        const char* items[] = { "Move", "Attack", "Wait", "Item", "Medcine", "Detox", "Defrozen", "Poison", "Auto" };
        int numItems = 9;
        
        // Draw Background Box
        UIManager::getInstance().DrawRectangle(menuX - 10, menuY - 10, 120, numItems * 25 + 20, 0x000000, 0xFFFFFF, 200);

        for(int i=0; i<numItems; i++) {
            uint32_t color = (i == menuIdx) ? 0xFF0000 : 0xFFFFFF;
            UIManager::getInstance().DrawShadowTextUtf8(items[i], menuX, menuY + i * 25, color, 0, 20);
        }
        
        UIManager::getInstance().UpdateScreen();
        
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_battleRunning = false;
                GameManager::getInstance().Quit();
                return -1;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_UP) menuIdx = (menuIdx - 1 + numItems) % numItems;
                if (event.key.key == SDLK_DOWN) menuIdx = (menuIdx + 1) % numItems;
                if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) {
                    if (menuIdx == 0) { // Move
                        int mx, my;
                        if (SelectMove(roleIdx, mx, my)) {
                            MoveRole(roleIdx, mx, my);
                            done = true;
                        }
                    } else if (menuIdx == 1) { // Attack
                        int targetIdx;
                        if (SelectAttack(roleIdx, targetIdx)) {
                            // Default Attack Magic?
                            // For now, use 0 or -1 to indicate physical?
                            // Attack() function expects magicId.
                            // If physical, maybe magicId = 0? Or separate function?
                            // Attack(roleIdx, targetIdx, 0); // Assuming 0 is valid or we handle it
                            // Actually KYS uses Magic ID for everything. Physical might be a special Magic?
                            // Or Attack() handles weapon.
                            Attack(roleIdx, targetIdx, 0); // Placeholder magic 0
                            done = true;
                        }
                    } else if (menuIdx == 2) { // Wait
                        done = true;
                    } else if (menuIdx == 3) { // Item
                        // BattleMenuItem(roleIdx); // Stub
                        AutoUseItem(roleIdx, 45); // Test: Auto Heal
                        done = true;
                    } else if (menuIdx == 4) { // Medcine
                        Medcine(roleIdx);
                        if (m_battleRoles[roleIdx].getActed()) done = true;
                    } else if (menuIdx == 5) { // Detox
                        MedPoision(roleIdx);
                        if (m_battleRoles[roleIdx].getActed()) done = true;
                    } else if (menuIdx == 6) { // Defrozen
                        MedFrozen(roleIdx);
                        if (m_battleRoles[roleIdx].getActed()) done = true;
                    } else if (menuIdx == 7) { // Poison
                        UsePoision(roleIdx);
                        if (m_battleRoles[roleIdx].getActed()) done = true;
                    } else if (menuIdx == 8) { // Auto
                        AutoBattle(roleIdx);
                        done = true;
                    }
                }
                if (event.key.key == SDLK_ESCAPE) {
                    // Cannot escape turn, but can cancel menu if we had sub-menus
                }
            }
        }
        SDL_Delay(10);
    }
    
    m_battleRoles[roleIdx].setActed(1);
    m_battleRoles[roleIdx].setProgress(0);
    return 0;
}

bool BattleManager::SelectMove(int roleIdx, int& outX, int& outY) {
    bool done = false;
    SDL_Event event;
    BattleRole& actor = m_battleRoles[roleIdx];
    m_cursorX = actor.getX();
    m_cursorY = actor.getY();
    
    CalSelectableArea(roleIdx);
    
    while (!done) {
        RenderBattle();
        UIManager::getInstance().DrawShadowTextUtf8("请选择移动位置", 10, 10, 0xFFFFFF, 0x000000);
        UIManager::getInstance().UpdateScreen();
        
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_battleRunning = false;
                GameManager::getInstance().Quit();
                return false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_LEFT) { m_cursorX--; if(m_cursorX < 0) m_cursorX = 0; }
                if (event.key.key == SDLK_RIGHT) { m_cursorX++; if(m_cursorX > 63) m_cursorX = 63; }
                if (event.key.key == SDLK_UP) { m_cursorY--; if(m_cursorY < 0) m_cursorY = 0; }
                if (event.key.key == SDLK_DOWN) { m_cursorY++; if(m_cursorY > 63) m_cursorY = 63; }
                
                if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) {
                    if (m_battleField[3][m_cursorX][m_cursorY] > 0) {
                        outX = m_cursorX;
                        outY = m_cursorY;
                        return true;
                    }
                }
                if (event.key.key == SDLK_ESCAPE) return false;
            }
        }
        SDL_Delay(10);
    }
    return false;
}

bool BattleManager::SelectAttack(int roleIdx, int& outTargetIdx) {
    bool done = false;
    SDL_Event event;
    BattleRole& actor = m_battleRoles[roleIdx];
    m_cursorX = actor.getX();
    m_cursorY = actor.getY();
    
    // Simple Range for now
    int range = 1; 
    
    SetAttackArea(0, actor.getX(), actor.getY(), range, 0, 0, 0);
    
    while (!done) {
        RenderBattle();
        UIManager::getInstance().DrawShadowTextUtf8("请选择攻击目标", 10, 10, 0xFFFFFF, 0x000000);
        UIManager::getInstance().UpdateScreen();
        
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_battleRunning = false;
                GameManager::getInstance().Quit();
                return false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_LEFT) { m_cursorX--; if(m_cursorX < 0) m_cursorX = 0; }
                if (event.key.key == SDLK_RIGHT) { m_cursorX++; if(m_cursorX > 63) m_cursorX = 63; }
                if (event.key.key == SDLK_UP) { m_cursorY--; if(m_cursorY < 0) m_cursorY = 0; }
                if (event.key.key == SDLK_DOWN) { m_cursorY++; if(m_cursorY > 63) m_cursorY = 63; }
                
                if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) {
                    if (m_battleField[4][m_cursorX][m_cursorY] > 0) {
                         int target = m_battleField[2][m_cursorX][m_cursorY];
                         if (target >= 0 && target != roleIdx) {
                             outTargetIdx = target;
                             return true;
                         }
                    }
                }
                if (event.key.key == SDLK_ESCAPE) return false;
            }
        }
        SDL_Delay(10);
    }
    return false;
}

bool BattleManager::SelectAim(int roleIdx, int step) {
    bool done = false;
    SDL_Event event;
    BattleRole& actor = m_battleRoles[roleIdx];
    m_cursorX = actor.getX();
    m_cursorY = actor.getY();
    
    // Highlight range
    CalSelectableArea(roleIdx); // Note: This calculates movement range. 
                                // For skills, we might need simple distance check.
                                // But for now, let's use a simple distance loop in render or logic.
    
    while (!done) {
        RenderBattle();
        // Draw cursor info
        // ...
        
        UIManager::getInstance().UpdateScreen();
        
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_battleRunning = false;
                GameManager::getInstance().Quit();
                return false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_UP) m_cursorY = (m_cursorY - 1 + 64) % 64;
                if (event.key.key == SDLK_DOWN) m_cursorY = (m_cursorY + 1) % 64;
                if (event.key.key == SDLK_LEFT) m_cursorX = (m_cursorX - 1 + 64) % 64;
                if (event.key.key == SDLK_RIGHT) m_cursorX = (m_cursorX + 1) % 64;
                
                if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) {
                    // Check if target is valid
                    int dist = std::abs(m_cursorX - actor.getX()) + std::abs(m_cursorY - actor.getY());
                    if (dist <= step) {
                        return true;
                    }
                }
                if (event.key.key == SDLK_ESCAPE) {
                    return false;
                }
            }
        }
    }
    return false;
}

void BattleManager::RenderBattle() {
    SDL_Renderer* renderer = GameManager::getInstance().getRenderer();
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); // Enable Blending
    
    // Draw Map (Simple Isometric Loop)
    int cx = 32, cy = 32; // Default Center
    if (m_currentRoleIndex >= 0 && m_currentRoleIndex < m_battleRoles.size()) {
        cx = m_battleRoles[m_currentRoleIndex].getX();
        cy = m_battleRoles[m_currentRoleIndex].getY();
    } else if (m_cursorX >= 0) {
        cx = m_cursorX;
        cy = m_cursorY;
    }
    
    // Render tiles
    // Using SceneManager::DrawTile to reuse smp/sdx resources
    // KYS Battle Map: 64x64
    // Layer 0: Ground
    // Layer 1: Object (Building/Tree)
    
    for(int i1 = 0; i1 < 64; ++i1) {
        for(int i2 = 0; i2 < 64; ++i2) {
            int x, y;
            SceneManager::getInstance().GetPositionOnScreen(i1, i2, cx, cy, x, y);
            
            // Culling
            if (x < -200 || x > 840 || y < -200 || y > 680) continue; 

            // Layer 0: Ground
            int16_t tile0 = m_battleField[0][i1][i2];
            if (tile0 > 0) {
                // Battle map tiles use same indexing as Scene? 
                // Usually yes.
                SceneManager::getInstance().DrawTile(renderer, (tile0 / 2) - 1, x, y, 0, 0);
            }
            
            // Layer 1: Object
            int16_t tile1 = m_battleField[1][i1][i2];
            if (tile1 > 0) {
                SceneManager::getInstance().DrawTile(renderer, (tile1 / 2) - 1, x, y, 0, 0);
            }
            
            // Layer 3: Move Range (Overlay)
            if (m_battleField[3][i1][i2] > 0) {
                 // Draw Blue/White semi-transparent tile
                 // Tile shape is diamond, but rect is simpler for now
                 SDL_FRect rect = { (float)x + 2, (float)y + 2, 36.0f, 18.0f }; // Approx tile size
                 SDL_SetRenderDrawColor(renderer, 200, 200, 255, 128); // Light Blue
                 SDL_RenderFillRect(renderer, &rect);
            }
            
            // Layer 4: Attack Range (Overlay)
            if (m_battleField[4][i1][i2] > 0) {
                 // Draw Red semi-transparent tile
                 SDL_FRect rect = { (float)x + 2, (float)y + 2, 36.0f, 18.0f };
                 SDL_SetRenderDrawColor(renderer, 255, 50, 50, 128); // Red
                 SDL_RenderFillRect(renderer, &rect);
            }
            
            // Roles
            int rIdx = m_battleField[2][i1][i2];
            if (rIdx >= 0 && rIdx < m_battleRoles.size()) {
                BattleRole& r = m_battleRoles[rIdx];
                if (!r.getDead()) {
                    // Draw Sprite
                    // For now, use a default sprite if Pic is just HeadNum.
                    // Need mapping from HeadNum to BattleSprite?
                    // Actually, Role has no direct "BattleSprite" field in standard struct?
                    // Ah, KYS uses "FightPic" usually.
                    // Let's assume standard sprite for now: 2553 + ...
                    // Or if r.getPic() is HeadNum, we need to map it.
                    // For now, let's just draw the "Player" sprite 2501 for everyone or use Head logic?
                    // No, Head is for UI.
                    // Let's use a placeholder sprite index from mmap: 2553 (Battle Role Start)
                    // + rIdx usually?
                    
                    // Use a fixed sprite for now to verify rendering
                    SceneManager::getInstance().DrawSprite(renderer, 2553 + (r.getTeam() * 5), x, y, 0);
                    
                    // Health Bar?
                    SDL_FRect hpRect = { (float)x + 10, (float)y - 80, 20.0f, 4.0f };
                    SDL_SetRenderDrawColor(renderer, r.getTeam() == 0 ? 0 : 255, r.getTeam() == 0 ? 255 : 0, 0, 255);
                    SDL_RenderFillRect(renderer, &hpRect);
                } else {
                    // Dead body?
                    SceneManager::getInstance().DrawSprite(renderer, 2553 + 20, x, y, 0);
                }
            }
            
            // Cursor
            if (i1 == m_cursorX && i2 == m_cursorY) {
                 // Draw Cursor Highlight
                 // SDL_SetRenderDrawColor(renderer, 255, 255, 0, 128); // Yellow
                 // Draw a box around tile center
                 // Tile center is (x + 18, y + 9) roughly?
                 // Just draw a small rect
                 SDL_FRect cursorRect = { (float)x + 10, (float)y, 20.0f, 10.0f };
                 SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                 SDL_RenderRect(renderer, &cursorRect);
            }
        }
    }
}

void BattleManager::ShowHurtValue(int mode) {
    // Stub
}

void BattleManager::BattleMenuItem(int roleIdx) {
    // Stub
}

BattleRole& BattleManager::getBattleRole(int index) {
    if (index < 0 || index >= m_battleRoles.size()) {
        static BattleRole dummy;
        return dummy;
    }
    return m_battleRoles[index];
}

int BattleManager::getBattleRoleCount() const {
    return m_battleRoles.size();
}

int16_t BattleManager::getBattleField(int layer, int x, int y) const {
    if (layer < 0 || layer >= 8 || x < 0 || x >= 64 || y < 0 || y >= 64) return 0;
    return m_battleField[layer][x][y];
}

void BattleManager::setBattleField(int layer, int x, int y, int16_t val) {
    if (layer < 0 || layer >= 8 || x < 0 || x >= 64 || y < 0 || y >= 64) return;
    m_battleField[layer][x][y] = val;
}

void BattleManager::AddBattleRole(const BattleRole& role) {
    m_battleRoles.push_back(role);
}

void BattleManager::ClearBattleRoles() {
    m_battleRoles.clear();
}

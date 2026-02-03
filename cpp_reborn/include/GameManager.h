#pragma once
#include <vector>
#include <memory>
#include <string>
#include <SDL3/SDL.h>
#include "Role.h"
#include "Item.h"
#include "Scene.h"
#include "Magic.h"
#include "PicLoader.h"

class GameManager {
    // Global Variables (x50 array from Pascal)
    // Range: -32768 to 32767 (mapped to 0..65535)
    std::vector<int16_t> m_x50;

public:
    static GameManager& getInstance();

    int16_t getX50(int index) const {
        if (index < -32768 || index > 32767) return 0;
        return m_x50[index + 32768];
    }

    void setX50(int index, int16_t value) {
        if (index < -32768 || index > 32767) return;
        m_x50[index + 32768] = value;
    }

    bool Init();
    void Run();
    void Quit();

    // Data Access
    Role& getRole(int index);
    int getRoleCount() const { return (int)m_roles.size(); }
    Item& getItem(int index);
    int getItemCount() const { return (int)m_items.size(); }
    Scene& getScene(int index);
    Magic& getMagic(int index);
    int getMagicCount() const { return (int)m_magics.size(); }
    PicImage* getHead(int index); // Get cached head image
    
    // Global State Helpers
    void setMainMapPosition(int x, int y);
    void setCameraPosition(int x, int y); // Separate Camera Position
    
    // Render Helper
    SDL_Surface* getScreenSurface() { return m_screenSurface; }
    void RenderScreenTo(SDL_Renderer* renderer);
    void getMainMapPosition(int& x, int& y) const { x = m_mainMapX; y = m_mainMapY; }
    void getCameraPosition(int& x, int& y) const { x = m_cameraX; y = m_cameraY; }
    void setMainMapFace(int face) { m_mainMapFace = face; }
    int getMainMapFace() const { return m_mainMapFace; }
    
    // Walk Animation
    // KYS sprites typically have 6 or 7 frames.
    // Frame 0 is usually standing.
    // Frames 1-6 are walking cycle.
    // Or 0-5.
    // If user sees "abnormal frame", maybe 7 is too many and it wraps into next sprite?
    // Let's try reducing to 6 frames (0..5).
    void updateWalkFrame() { m_walkFrame = (m_walkFrame + 1) % 6; } 
    void resetWalkFrame() { m_walkFrame = 0; }
    int getWalkFrame() const { return m_walkFrame; }

    void enterScene(int sceneId);
    int getCurrentSceneId() const { return m_currentSceneId; }
    
    // Screen Access
    SDL_Renderer* getRenderer() { return m_renderer; }

    // Item Usage
    void EatOneItem(int roleNum, int itemId, int where = 0);
    int GetRoleMedcine(int roleNum, bool checkEquip = true); // Helper for medicine power
    
    // Inventory & Party
    void AddItem(int itemId, int amount);
    void useItem(int itemId);
    int getItemAmount(int itemId);
    const std::vector<InventoryItem>& getItemList() const { return m_inventory; }
    void JoinParty(int roleId);
    void LeaveParty(int roleId);
    void Rest();
    
    // Accessors
    const std::vector<int>& getTeamList() const { return m_teamList; }

    // Helper to load initial data
    void loadData(const std::string& savePrefix);

    // Save Path
    std::string m_savePath;

    // Save/Load
    void SaveGame(int slot);
    void LoadGame(int slot);

    // Game Logic
    bool GetEquipState(int roleIdx, int state);
    
    // Attribute Helpers
    int GetGongtiLevel(int roleIdx, int magicId);
    bool GetGongtiState(int roleIdx, int state);
    int GetRoleMedPoi(int roleIdx, bool checkEquip = true);
    int GetRoleUsePoi(int roleIdx, bool checkEquip = true);
    int GetRoleDefPoi(int roleIdx, bool checkEquip = true);
    int GetRoleFist(int roleIdx, bool checkEquip = true);
    int GetRoleSword(int roleIdx, bool checkEquip = true);
    int GetRoleKnife(int roleIdx, bool checkEquip = true);
    int GetRoleUnusual(int roleIdx, bool checkEquip = true);
    int GetRoleHidWeapon(int roleIdx, bool checkEquip = true);

    // Testing Helpers
    void addMagicForTest(const Magic& m) { m_magics.push_back(m); }
    void addRoleForTest(const Role& r) { m_roles.push_back(r); }
    void clearDataForTest() { m_magics.clear(); m_roles.clear(); m_items.clear(); }

private:
    GameManager();
    ~GameManager(); // Changed to non-default to handle SDL cleanup
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    // Game Data
    std::vector<Role> m_roles;
    std::vector<Item> m_items;
    // std::vector<Scene> m_scenes; // Moved to SceneManager
    std::vector<Magic> m_magics;
    std::vector<PicImage> m_heads; // Cached head images
    
    std::vector<int> m_teamList; // Stores role IDs of current party members
    std::vector<InventoryItem> m_inventory;
    
    // Runtime State
    enum class GameState {
        TitleScreen,
        CharacterCreation,
        Roaming,
        Battle,
        SystemMenu,
        InventoryMenu
    };
    GameState m_currentState = GameState::TitleScreen;

    bool m_isRunning;
    int m_currentSceneId;
    int m_mainMapX, m_mainMapY;
    int m_savedWorldX = 0, m_savedWorldY = 0; // Backup for world coordinates when entering a scene
    int m_cameraX, m_cameraY;
    int m_mainMapFace = 0;
    int m_walkFrame = 0;
    int16_t m_inShip = 0;
    int16_t m_shipX = 0, m_shipY = 0, m_shipFace = 0;
    int16_t m_subMapFace = 0;
    int16_t m_time = 0, m_timeEvent = 0, m_randomEvent = 0;
    int16_t m_gameTime = 0;
    bool m_playedTitleAnim = false;
    
    // System Menu State
    int m_systemMenuSelection = 0;

    // SDL Resources
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Surface* m_screenSurface;
    SDL_Texture* m_screenTexture;

    // Helper for Character Creation
    std::string m_characterCreationNameUtf8;
    bool m_characterCreationTextInputActive = false;
    void RandomizeRoleStats(Role& role);
    void InitNewGame();
    
    // Title Menu Helpers
    int m_titleMenuSelection = 0;
    void DrawTitleMenu();

    void UpdateTitleScreen();
    void UpdateCharacterCreation();
    void UpdateSystemMenu();
    void UpdateInventoryMenu();
    
public:
    void UpdateRoaming(); // Make public for EventManager to call
private:
    // 大地图行走与入口检测
    bool CanWalkWorld(int x, int y);
    bool CheckWorldEntrance();
};

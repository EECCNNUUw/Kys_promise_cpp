#define NOMINMAX
#include "GameManager.h"
#include "SceneManager.h"
#include "UIManager.h"
#include "BattleManager.h"
#include "EventManager.h"
#include "FileLoader.h"
#include "PicLoader.h"
#include "TextManager.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <random>
#include <direct.h> // For _getcwd on Windows
#include <io.h>     // For access
#define getcwd _getcwd
#define access _access

namespace {
    void PopBackUtf8(std::string& s) {
        if (s.empty()) return;
        size_t i = s.size() - 1;
        while (i > 0 && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) {
            --i;
        }
        s.erase(i);
    }
}

GameManager& GameManager::getInstance() {
    static GameManager instance;
    return instance;
}

GameManager::GameManager() 
    : m_window(nullptr), m_renderer(nullptr), m_screenSurface(nullptr), m_screenTexture(nullptr),
      m_isRunning(false), m_currentSceneId(0), m_mainMapX(0), m_mainMapY(0), 
      m_cameraX(0), m_cameraY(0), m_mainMapFace(0), m_walkFrame(0), m_playedTitleAnim(false),
      m_currentState(GameState::TitleScreen), m_systemMenuSelection(0)
{
    m_x50.resize(65536, 0); // -32768 to 32767 mapped to 0..65535
}

GameManager::~GameManager() {
    Quit();
}

bool GameManager::Init() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!TTF_Init()) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << SDL_GetError() << std::endl;
        return false;
    }

    m_window = SDL_CreateWindow("KYS Promise (C++ Refactor)", 640, 480, 0); 
    if (!m_window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, NULL); // SDL3 defaults are good
    if (!m_renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    SDL_SetRenderVSync(m_renderer, 1); // Enable VSync

    // Logical resolution
    // SDL3: SDL_SetRenderLogicalPresentation(renderer, w, h, mode)
    SDL_SetRenderLogicalPresentation(m_renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    m_screenSurface = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_ARGB8888);
    m_screenTexture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 640, 480);

    if (!UIManager::getInstance().Init(m_renderer, m_window)) {
        std::cerr << "Failed to init UIManager" << std::endl;
        return false;
    }

    // Initialize Subsystems
    if (!SceneManager::getInstance().Init()) {
        std::cerr << "Failed to init SceneManager" << std::endl;
        return false;
    }

    if (!EventManager::getInstance().Init()) {
        std::cerr << "Failed to init EventManager" << std::endl;
        return false;
    }

    if (!BattleManager::getInstance().Init()) {
        std::cerr << "Failed to init BattleManager" << std::endl;
        return false;
    }

    // Find Save Directory
    std::string savePrefix = "../save/";
    if (access((savePrefix + "ranger.grp").c_str(), 0) != 0) {
        savePrefix = "../../save/";
        if (access((savePrefix + "ranger.grp").c_str(), 0) != 0) {
            savePrefix = "../../../save/";
             if (access((savePrefix + "ranger.grp").c_str(), 0) != 0) {
                 savePrefix = "save/"; // Try current dir
             }
        }
    }
    
    // Convert to absolute path to bypass FileLoader's resource prefixing
    char absPath[1024];
    if (_fullpath(absPath, savePrefix.c_str(), 1024) != NULL) {
        savePrefix = std::string(absPath) + "\\";
    }
    
    // Store save path for later use (e.g. InitNewGame)
    m_savePath = savePrefix;
    
    std::cout << "[GameManager] Discovered Save Path: " << savePrefix << std::endl;

    // Load initial data
    std::cout << "[GameManager] Loading Scene Data from " << savePrefix << std::endl;
    std::string alldefPath = savePrefix + "alldef.grp";
    if (!SceneManager::getInstance().LoadEventData(alldefPath)) {
         std::cerr << "Failed to load Event Data from " << alldefPath << ", trying resource path 'alldef.grp'..." << std::endl;
         if (!SceneManager::getInstance().LoadEventData("alldef.grp")) {
             std::cerr << "CRITICAL: Failed to load alldef.grp from anywhere!" << std::endl;
         }
    }
    
    std::string allsinPath = savePrefix + "allsin.grp";
    if (!SceneManager::getInstance().LoadMapData(allsinPath)) {
         std::cerr << "Failed to load Map Data from " << allsinPath << ", trying resource path 'allsin.grp'..." << std::endl;
         if (!SceneManager::getInstance().LoadMapData("allsin.grp")) {
             std::cerr << "CRITICAL: Failed to load allsin.grp from anywhere!" << std::endl;
         }
    }

    loadData(savePrefix);

    m_currentState = GameState::TitleScreen;
    m_systemMenuSelection = 0;

    m_isRunning = true;
    return true;
}

void GameManager::loadData(const std::string& savePrefix) {
    LoadGame(0);
}

void GameManager::SaveGame(int slot) {
    std::string filename = (slot == 0) ? "ranger" : "R" + std::to_string(slot);
    std::string grpPath = m_savePath + filename + ".grp";
    std::string idxPath = m_savePath + "ranger.idx"; 

    std::ifstream idxFile(idxPath, std::ios::binary);
    if (!idxFile) {
        std::cerr << "SaveGame: Cannot open " << idxPath << std::endl;
        return;
    }

    int32_t RoleOffset, ItemOffset, SceneOffset, MagicOffset, WeiShopOffset, TotalLen;
    idxFile.read((char*)&RoleOffset, 4);
    idxFile.read((char*)&ItemOffset, 4);
    idxFile.read((char*)&SceneOffset, 4);
    idxFile.read((char*)&MagicOffset, 4);
    idxFile.read((char*)&WeiShopOffset, 4);
    idxFile.read((char*)&TotalLen, 4);
    idxFile.close();

    std::ofstream grpFile(grpPath, std::ios::binary);
    if (!grpFile) {
        std::cerr << "SaveGame: Cannot create " << grpPath << std::endl;
        return;
    }

    auto write16 = [&](int16_t val) {
        grpFile.write((char*)&val, 2);
    };

    write16(m_inShip);
    if (m_currentSceneId == 0) write16(-1);
    else write16((int16_t)m_currentSceneId);

    write16((int16_t)m_mainMapY);
    write16((int16_t)m_mainMapX);
    write16((int16_t)m_cameraY);
    write16((int16_t)m_cameraX);
    write16((int16_t)m_mainMapFace);
    write16(m_shipX);
    write16(m_shipY);
    write16(m_time);
    write16(m_timeEvent);
    write16(m_randomEvent);
    write16(m_subMapFace);
    write16(m_shipFace);
    write16(m_gameTime);

    for (int i = 0; i < MAX_TEAM_SIZE; ++i) {
        int val = (i < m_teamList.size()) ? m_teamList[i] : -1;
        write16((int16_t)val);
    }

    for (int i = 0; i < MAX_ITEM_AMOUNT; ++i) {
        if (i < m_inventory.size()) {
            write16(m_inventory[i].id);
            write16(m_inventory[i].amount);
        } else {
            write16(0);
            write16(0);
        }
    }

    int roleBytesToWrite = ItemOffset - RoleOffset;
    int roleBytesWritten = 0;
    for (const auto& role : m_roles) {
        if (roleBytesWritten + ROLE_DATA_SIZE * 2 <= roleBytesToWrite) {
            grpFile.write((char*)role.getRawData(), ROLE_DATA_SIZE * 2);
            roleBytesWritten += ROLE_DATA_SIZE * 2;
        } else break;
    }
    while (roleBytesWritten < roleBytesToWrite) {
        char zero = 0;
        grpFile.write(&zero, 1);
        roleBytesWritten++;
    }

    int itemBytesToWrite = SceneOffset - ItemOffset;
    int itemBytesWritten = 0;
    for (const auto& item : m_items) {
        if (itemBytesWritten + ITEM_DATA_SIZE * 2 <= itemBytesToWrite) {
            grpFile.write((char*)item.getRawData(), ITEM_DATA_SIZE * 2);
            itemBytesWritten += ITEM_DATA_SIZE * 2;
        } else break;
    }
    while (itemBytesWritten < itemBytesToWrite) {
        char zero = 0;
        grpFile.write(&zero, 1);
        itemBytesWritten++;
    }

    int sceneBytesToWrite = MagicOffset - SceneOffset;
    int sceneBytesWritten = 0;
    for (int i = 0; i < 200; ++i) { // Assume max 200 scenes to iterate
         Scene* s = SceneManager::getInstance().GetScene(i);
         if (s) {
             if (sceneBytesWritten + SCENE_DATA_SIZE * 2 <= sceneBytesToWrite) {
                 grpFile.write((char*)s->getRawData(), SCENE_DATA_SIZE * 2);
                 sceneBytesWritten += SCENE_DATA_SIZE * 2;
             }
         }
    }
    while (sceneBytesWritten < sceneBytesToWrite) {
        char zero = 0;
        grpFile.write(&zero, 1);
        sceneBytesWritten++;
    }

    int magicBytesToWrite = WeiShopOffset - MagicOffset;
    int magicBytesWritten = 0;
    for (const auto& magic : m_magics) {
        if (magicBytesWritten + MAGIC_DATA_SIZE * 2 <= magicBytesToWrite) {
            grpFile.write((char*)magic.getRawData(), MAGIC_DATA_SIZE * 2);
            magicBytesWritten += MAGIC_DATA_SIZE * 2;
        } else break;
    }
    while (magicBytesWritten < magicBytesToWrite) {
        char zero = 0;
        grpFile.write(&zero, 1);
        magicBytesWritten++;
    }

    int shopBytesToWrite = TotalLen - WeiShopOffset;
    std::vector<char> zeros(shopBytesToWrite, 0);
    grpFile.write(zeros.data(), shopBytesToWrite);

    grpFile.close();

    std::string sFilename = (slot == 0) ? "allsin.grp" : "S" + std::to_string(slot) + ".grp";
    std::string dFilename = (slot == 0) ? "alldef.grp" : "D" + std::to_string(slot) + ".grp";
    
    SceneManager::getInstance().SaveMapData(m_savePath + sFilename);
    SceneManager::getInstance().SaveEventData(m_savePath + dFilename);
    
    std::cout << "Game Saved to Slot " << slot << std::endl;
}

void GameManager::LoadGame(int slot) {
    std::string filename = (slot == 0) ? "ranger" : "R" + std::to_string(slot);
    std::string grpPath = m_savePath + filename + ".grp";
    std::string idxPath = m_savePath + "ranger.idx";

    std::ifstream idxFile(idxPath, std::ios::binary);
    if (!idxFile) {
        std::cerr << "LoadGame: Cannot open " << idxPath << std::endl;
        return;
    }

    int32_t RoleOffset, ItemOffset, SceneOffset, MagicOffset, WeiShopOffset, TotalLen;
    idxFile.read((char*)&RoleOffset, 4);
    idxFile.read((char*)&ItemOffset, 4);
    idxFile.read((char*)&SceneOffset, 4);
    idxFile.read((char*)&MagicOffset, 4);
    idxFile.read((char*)&WeiShopOffset, 4);
    idxFile.read((char*)&TotalLen, 4);
    idxFile.close();

    std::ifstream grpFile(grpPath, std::ios::binary);
    if (!grpFile) {
        if (slot == 0) {
            grpPath = m_savePath + "Ranger.grp";
            grpFile.open(grpPath, std::ios::binary);
        }
        if (!grpFile) {
            std::cerr << "LoadGame: Cannot open " << grpPath << std::endl;
            return;
        }
    }

    auto read16 = [&](int16_t& val) {
        grpFile.read((char*)&val, 2);
    };

    read16(m_inShip);
    int16_t tempWhere;
    read16(tempWhere);
    if (tempWhere < 0) tempWhere = 0;
    m_currentSceneId = tempWhere;
    SceneManager::getInstance().SetCurrentScene(m_currentSceneId);

    read16((int16_t&)m_mainMapY);
    read16((int16_t&)m_mainMapX);
    read16((int16_t&)m_cameraY);
    read16((int16_t&)m_cameraX);
    read16((int16_t&)m_mainMapFace);
    read16(m_shipX);
    read16(m_shipY);
    read16(m_time);
    read16(m_timeEvent);
    read16(m_randomEvent);
    read16(m_subMapFace);
    read16(m_shipFace);
    read16(m_gameTime);

    m_teamList.resize(MAX_TEAM_SIZE);
    for(int i=0; i<MAX_TEAM_SIZE; ++i) {
        read16((int16_t&)m_teamList[i]);
    }

    m_inventory.resize(MAX_ITEM_AMOUNT);
    for(int i=0; i<MAX_ITEM_AMOUNT; ++i) {
        read16(m_inventory[i].id);
        read16(m_inventory[i].amount);
    }

    grpFile.seekg(RoleOffset, std::ios::beg);
    int roleSize = (ItemOffset - RoleOffset) / (ROLE_DATA_SIZE * 2);
    m_roles.resize(roleSize);
    for(int i=0; i<roleSize; ++i) {
        std::vector<int16_t> buffer(ROLE_DATA_SIZE);
        grpFile.read((char*)buffer.data(), ROLE_DATA_SIZE * 2);
        m_roles[i].setDataVector(buffer);
    }

    grpFile.seekg(ItemOffset, std::ios::beg);
    int itemSize = (SceneOffset - ItemOffset) / (ITEM_DATA_SIZE * 2);
    m_items.resize(itemSize);
    for(int i=0; i<itemSize; ++i) {
        std::vector<int16_t> buffer(ITEM_DATA_SIZE);
        grpFile.read((char*)buffer.data(), ITEM_DATA_SIZE * 2);
        m_items[i].setDataVector(buffer);
    }

    grpFile.seekg(SceneOffset, std::ios::beg);
    int sceneSize = (MagicOffset - SceneOffset) / (SCENE_DATA_SIZE * 2);
    std::vector<Scene> scenes(sceneSize);
    for(int i=0; i<sceneSize; ++i) {
        std::vector<int16_t> buffer(SCENE_DATA_SIZE);
        grpFile.read((char*)buffer.data(), SCENE_DATA_SIZE * 2);
        scenes[i].setDataVector(buffer);
    }
    SceneManager::getInstance().SetScenes(scenes);

    grpFile.seekg(MagicOffset, std::ios::beg);
    int magicSize = (WeiShopOffset - MagicOffset) / (MAGIC_DATA_SIZE * 2);
    m_magics.resize(magicSize);
    for(int i=0; i<magicSize; ++i) {
        std::vector<int16_t> buffer(MAGIC_DATA_SIZE);
        grpFile.read((char*)buffer.data(), MAGIC_DATA_SIZE * 2);
        m_magics[i].setDataVector(buffer);
    }

    grpFile.close();

    std::string sFilename = (slot == 0) ? "allsin.grp" : "S" + std::to_string(slot) + ".grp";
    std::string dFilename = (slot == 0) ? "alldef.grp" : "D" + std::to_string(slot) + ".grp";
    
    if (!SceneManager::getInstance().LoadMapData(m_savePath + sFilename)) {
        if (slot == 0) SceneManager::getInstance().LoadMapData(m_savePath + "allsin.grp");
    }
    if (!SceneManager::getInstance().LoadEventData(m_savePath + dFilename)) {
        if (slot == 0) SceneManager::getInstance().LoadEventData(m_savePath + "alldef.grp");
    }
    
    std::cout << "Game Loaded from Slot " << slot << std::endl;
}

void GameManager::InitNewGame() {
    // KYS New Game: Load initial state from ranger.grp
    // User feedback indicates that ranger.grp acts as the "New Game Save".
    
    // 初始化新游戏
    // 对应 Pascal 的 NewGame 流程
    std::string loadPath = m_savePath;
    if (loadPath.empty()) {
        loadPath = "save/"; // Fallback
    }
    
    std::string preservedHeroNameGbk;
    if (!m_characterCreationNameUtf8.empty()) {
        preservedHeroNameGbk = TextManager::getInstance().utf8ToGbk(m_characterCreationNameUtf8);
    } else if (getRoleCount() > 0) {
        preservedHeroNameGbk = getRole(0).getName();
    }

    // Reload data (including Header which sets Scene/Pos)
    loadData(loadPath);

    if (!preservedHeroNameGbk.empty() && getRoleCount() > 0) {
        getRole(0).setName(preservedHeroNameGbk);
    }
    
    // OVERRIDE for New Game: Force start in Scene 0 (Temple)
    // 修正：新游戏强制从场景0（圣堂/软体娃娃处）开始
    // ranger.grp 的头部可能保存的是大地图状态(-1)，这对新游戏是不正确的。
    // 实际的开场剧情由场景0的事件101处理。
    m_currentSceneId = 0; 
    
    // Updated based on user feedback: Correct start position in Scene 0 is (38, 38)
    m_mainMapX = 38;
    m_mainMapY = 38;
    
    SceneManager::getInstance().SetCurrentScene(m_currentSceneId);
    
    // Sync Camera to Player
    setMainMapPosition(m_mainMapX, m_mainMapY);
    
    // Reset Inventory if not loaded from ranger.grp (Header doesn't contain items, but item array does)
    // Actually, loadData reloads m_items.
    // But does m_items contain the *amount*?
    // In KYS, RItem array usually contains item definitions + amount? 
    // Wait, KYS original RItem has 'Number' field?
    // Let's assume loadData handles it.
    
    // Initial Event Check to trigger opening cutscene (Auto-Run)
    // For New Game, we explicitly trigger Event 101 (Opening)
    // This is critical because ranger.grp might place us on World Map (-1) without active events.
    // 触发开场事件 (Event 101)
    std::cout << "[InitNewGame] Triggering Opening Event 101..." << std::endl;
    EventManager::getInstance().ExecuteEvent(101);
    
    // Also check for auto-events in the current scene (just in case)
    // EventManager::getInstance().CheckAutoEvents(m_currentSceneId);
    
    // Debug: Print initial position
    std::cout << "[InitNewGame] Scene: " << m_currentSceneId << " Pos: (" << m_mainMapX << ", " << m_mainMapY << ")" << std::endl;
    
    // Debug: Check Tile at position
    int16_t tile = SceneManager::getInstance().GetSceneTile(m_currentSceneId, 0, m_mainMapX, m_mainMapY);
    std::cout << "[InitNewGame] Tile at (" << m_mainMapX << ", " << m_mainMapY << ") Layer 0: " << tile << std::endl;
}

void GameManager::RandomizeRoleStats(Role& role) {
    // Simple randomization for testing
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(20, 30);
    std::uniform_int_distribution<> hp_dis(50, 100);

    role.setMaxHP(hp_dis(gen));
    role.setCurrentHP(role.getMaxHP());
    role.setMaxMP(hp_dis(gen));
    role.setCurrentMP(role.getMaxMP());

    role.setAttack(dis(gen));
    role.setDefence(dis(gen));
    role.setSpeed(dis(gen));
    role.setMedicine(dis(gen));
    role.setFist(dis(gen));
}

void GameManager::Run() {
    while (m_isRunning) {
        SDL_RenderClear(m_renderer);

        if (m_currentState == GameState::TitleScreen) {
            UpdateTitleScreen();
        } else if (m_currentState == GameState::CharacterCreation) {
            UpdateCharacterCreation();
        } else if (m_currentState == GameState::Roaming) {
            UpdateRoaming();
        } else if (m_currentState == GameState::Battle) {
            BattleManager::getInstance().RunBattle();
            m_currentState = GameState::Roaming; 
        } else if (m_currentState == GameState::SystemMenu) {
            UpdateSystemMenu();
        } else if (m_currentState == GameState::InventoryMenu) {
            UpdateInventoryMenu();
        }
        
        // Present is called in Update functions
        SDL_Delay(10);
    }
    std::cout << "Exiting Game Loop..." << std::endl;
}

void GameManager::Quit() {
    m_isRunning = false;
    
    // Cleanup Subsystems
    // SceneManager::getInstance().Cleanup(); // SceneManager does not have Cleanup
    // BattleManager::getInstance().Cleanup();
    // UIManager::getInstance().Cleanup(); // Managed by static instance but good to have explicit cleanup if needed

    if (m_screenTexture) {
        SDL_DestroyTexture(m_screenTexture);
        m_screenTexture = nullptr;
    }
    
    if (m_screenSurface) {
        SDL_DestroySurface(m_screenSurface);
        m_screenSurface = nullptr;
    }

    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    TTF_Quit();
    SDL_Quit();
}

void GameManager::UpdateTitleScreen() {
    if (!m_playedTitleAnim) {
        UIManager::getInstance().PlayTitleAnimation();
        m_playedTitleAnim = true;
    }
    
    
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_EVENT_QUIT) {
            m_isRunning = false;
        } else if (e.type == SDL_EVENT_KEY_DOWN) {
            switch (e.key.key) {
                case SDLK_UP:
                    m_titleMenuSelection--;
                    if (m_titleMenuSelection < 0) m_titleMenuSelection = 2;
                    break;
                case SDLK_DOWN:
                    m_titleMenuSelection++;
                    if (m_titleMenuSelection > 2) m_titleMenuSelection = 0;
                    break;
                case SDLK_RETURN:
                case SDLK_SPACE:
                    if (m_titleMenuSelection == 0) {
                        m_currentState = GameState::CharacterCreation;
                        RandomizeRoleStats(getRole(0));
                        m_characterCreationNameUtf8 = TextManager::getInstance().nameToUtf8(getRole(0).getName());
                        if (m_characterCreationNameUtf8.empty()) {
                            m_characterCreationNameUtf8 = "金先生";
                        }
                        getRole(0).setName(TextManager::getInstance().utf8ToGbk(m_characterCreationNameUtf8));
                        SDL_StartTextInput(m_window);
                        m_characterCreationTextInputActive = true;
                    } else if (m_titleMenuSelection == 1) {
                        UIManager::getInstance().ShowSaveLoadMenu(false);
                    } else if (m_titleMenuSelection == 2) {
                        m_isRunning = false;
                    }
                    break;
                case SDLK_ESCAPE:
                    m_isRunning = false;
                    break;
            }
        }
    }
    
    UIManager::getInstance().DrawTitleBackground();
    DrawTitleMenu();
    SDL_RenderPresent(m_renderer);
}

void GameManager::DrawTitleMenu() {
    const char* items[] = { "新 游 戏 (Start Game)", "载 入 进 度 (Load Game)", "离 开 游 戏 (Quit Game)" };
    int startX = 220;
    int startY = 300;
    int gapY = 40;
    
    for (int i = 0; i < 3; ++i) {
        uint32_t color = (i == m_titleMenuSelection) ? 0xFF0000 : 0xFFFFFF;
        UIManager::getInstance().DrawShadowTextUtf8(items[i], startX, startY + i * gapY, color, 0x000000, 24);
    }
}

void GameManager::UpdateCharacterCreation() {

    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_EVENT_QUIT) {
            m_isRunning = false;
            if (m_characterCreationTextInputActive) {
                SDL_StopTextInput(m_window);
                m_characterCreationTextInputActive = false;
            }
        } else if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_R || e.key.key == SDLK_SPACE) {
                RandomizeRoleStats(getRole(0));
            } else if (e.key.key == SDLK_BACKSPACE) {
                PopBackUtf8(m_characterCreationNameUtf8);
                getRole(0).setName(TextManager::getInstance().utf8ToGbk(m_characterCreationNameUtf8));
            } else if (e.key.key == SDLK_Y || e.key.key == SDLK_RETURN) {
                if (m_characterCreationTextInputActive) {
                    SDL_StopTextInput(m_window);
                    m_characterCreationTextInputActive = false;
                }
                getRole(0).setName(TextManager::getInstance().utf8ToGbk(m_characterCreationNameUtf8));
                InitNewGame();
                m_currentState = GameState::Roaming;
            } else if (e.key.key == SDLK_ESCAPE) {
                if (m_characterCreationTextInputActive) {
                    SDL_StopTextInput(m_window);
                    m_characterCreationTextInputActive = false;
                }
                m_currentState = GameState::TitleScreen;
            }
        } else if (e.type == SDL_EVENT_TEXT_INPUT) {
            std::string appended = m_characterCreationNameUtf8;
            appended += e.text.text;
            std::string gbk = TextManager::getInstance().utf8ToGbk(appended);
            if (!gbk.empty() && gbk.size() <= 9) {
                m_characterCreationNameUtf8 = std::move(appended);
                getRole(0).setName(gbk);
            }
        }
    }
    UIManager::getInstance().ShowCharacterCreation(getRole(0));
    SDL_RenderPresent(m_renderer);
}

void GameManager::UpdateRoaming() {
    EventManager::getInstance().CheckEvent(m_currentSceneId, m_mainMapX, m_mainMapY);

    int pendingEvent = EventManager::getInstance().GetPendingEvent();
    if (pendingEvent != -1) {
        EventManager::getInstance().ClearPendingEvent();
        EventManager::getInstance().ExecuteEvent(pendingEvent);
    }

    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_EVENT_QUIT) {
            m_isRunning = false;
        } else if (e.type == SDL_EVENT_KEY_DOWN) {
            int dx = 0, dy = 0;
            switch (e.key.key) {
                case SDLK_UP:    case SDLK_W: dx = -1; m_mainMapFace = 1; break;
                case SDLK_DOWN:  case SDLK_S: dx = 1;  m_mainMapFace = 0; break;
                case SDLK_LEFT:  case SDLK_A: dy = -1; m_mainMapFace = 2; break;
                case SDLK_RIGHT: case SDLK_D: dy = 1;  m_mainMapFace = 3; break;
                
                case SDLK_RETURN:
                case SDLK_SPACE:
                    {
                        int frontX = m_mainMapX;
                        int frontY = m_mainMapY;
                        switch(m_mainMapFace) {
                            case 0: frontX++; break; // Down
                            case 1: frontX--; break; // Up
                            case 2: frontY--; break; // Left
                            case 3: frontY++; break; // Right
                        }
                        EventManager::getInstance().CheckEvent(m_currentSceneId, frontX, frontY);
                    }
                    break;
                    
                case SDLK_C: 
                    if (!m_teamList.empty()) {
                        UIManager::getInstance().ShowStatus(m_teamList[0]);
                    }
                    break;
                
                case SDLK_ESCAPE:
                    UIManager::getInstance().ShowMenu();
                    break;
            }
            
            if (dx != 0 || dy != 0) {
                int nextX = m_mainMapX + dx;
                int nextY = m_mainMapY + dy;
                
                if (SceneManager::getInstance().CanWalk(nextX, nextY)) {
                    m_mainMapX = nextX;
                    m_mainMapY = nextY;
                    m_cameraX = m_mainMapX;
                    m_cameraY = m_mainMapY;
                    updateWalkFrame();
                    EventManager::getInstance().CheckEvent(m_currentSceneId, m_mainMapX, m_mainMapY);
                }
            }
        }
    }
    
    if (m_screenSurface) {
        SDL_FillSurfaceRect(m_screenSurface, NULL, 0x000000);
        SceneManager::getInstance().DrawScene(m_renderer, m_cameraX, m_cameraY);
        RenderScreenTo(m_renderer);
    }
    SDL_RenderPresent(m_renderer);
}

void GameManager::UpdateSystemMenu() {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_EVENT_QUIT) {
            m_isRunning = false;
        } else if (e.type == SDL_EVENT_KEY_DOWN) {
            switch (e.key.key) {
                case SDLK_UP:
                case SDLK_KP_8:
                    if (m_systemMenuSelection == 1 || m_systemMenuSelection == 2 || m_systemMenuSelection == 3) m_systemMenuSelection--;
                    else if (m_systemMenuSelection == 4) m_systemMenuSelection++;
                    else if (m_systemMenuSelection == 5) m_systemMenuSelection = 0;
                    break;
                    
                case SDLK_DOWN:
                case SDLK_KP_2:
                    if (m_systemMenuSelection == 0 || m_systemMenuSelection == 1 || m_systemMenuSelection == 2) m_systemMenuSelection++;
                    else if (m_systemMenuSelection == 5 || m_systemMenuSelection == 4) m_systemMenuSelection--;
                    break;
                    
                case SDLK_RIGHT:
                case SDLK_KP_6:
                    if (m_systemMenuSelection == 0) m_systemMenuSelection++;
                    else if (m_systemMenuSelection == 3 || m_systemMenuSelection == 4) m_systemMenuSelection--;
                    else if (m_systemMenuSelection == 5) m_systemMenuSelection = 0;
                    break;
                    
                case SDLK_LEFT:
                case SDLK_KP_4:
                    if (m_systemMenuSelection == 0) m_systemMenuSelection = 5;
                    else if (m_systemMenuSelection == 3 || m_systemMenuSelection == 2) m_systemMenuSelection++;
                    else if (m_systemMenuSelection == 5 || m_systemMenuSelection == 1) m_systemMenuSelection--;
                    break;

                case SDLK_RETURN:
                case SDLK_SPACE:
                    if (m_systemMenuSelection == 5) { // Item
                         m_currentState = GameState::InventoryMenu;
                    } else if (m_systemMenuSelection == 2) { // System
                         // TODO: Implement System Options
                    }
                    break;
                    
                case SDLK_ESCAPE:
                    m_currentState = GameState::Roaming;
                    break;
            }
        }
    }
    
    UIManager::getInstance().RenderMenuSystem(m_systemMenuSelection);
    SDL_RenderPresent(m_renderer);
}

void GameManager::UpdateInventoryMenu() {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_EVENT_QUIT) {
             m_isRunning = false;
        } else if (e.type == SDL_EVENT_KEY_DOWN) {
             if (e.key.key == SDLK_ESCAPE) {
                 m_currentState = GameState::SystemMenu;
             }
        }
    }
    
    // Draw Inventory (Placeholder)
    // UIManager::getInstance().RenderInventoryMenu(); 
    // Need to implement RenderInventoryMenu in UIManager first
    SDL_RenderClear(m_renderer);
    UIManager::getInstance().DrawShadowTextUtf8("Inventory Menu (TODO)", 200, 200, 0xFFFFFF, 0x000000, 30);
    SDL_RenderPresent(m_renderer);
}

void GameManager::RenderScreenTo(SDL_Renderer* renderer) {
    if (!renderer || !m_screenSurface || !m_screenTexture) return;
    SDL_UpdateTexture(m_screenTexture, NULL, m_screenSurface->pixels, m_screenSurface->pitch);
    SDL_RenderTexture(renderer, m_screenTexture, NULL, NULL);
}

Role& GameManager::getRole(int index) {
    if (index < 0 || index >= m_roles.size()) {
        static Role dummy; 
        return dummy; 
    }
    return m_roles[index];
}

Item& GameManager::getItem(int index) {
    if (index < 0 || index >= m_items.size()) {
        static Item dummy;
        return dummy;
    }
    return m_items[index];
}

Scene& GameManager::getScene(int index) {
    Scene* scene = SceneManager::getInstance().GetScene(index);
    if (scene) {
        return *scene;
    }
    static Scene dummy;
    return dummy;
}

Magic& GameManager::getMagic(int index) {
    if (index < 0 || index >= m_magics.size()) {
        static Magic dummy;
        return dummy;
    }
    return m_magics[index];
}

PicImage* GameManager::getHead(int index) {
    if (m_heads.empty()) {
        int count = PicLoader::getPicCount("resource/Heads.Pic");
        if (count > 0) {
            m_heads.resize(count);
        }
    }

    if (index >= 0 && index < m_heads.size()) {
        if (m_heads[index].surface == nullptr) {
            m_heads[index] = PicLoader::loadPic("resource/Heads.Pic", index);
        }
        return &m_heads[index];
    }
    return nullptr;
}

void GameManager::setCameraPosition(int x, int y) {
    m_cameraX = x;
    m_cameraY = y;
}

void GameManager::setMainMapPosition(int x, int y) {
    m_mainMapX = x;
    m_mainMapY = y;
    m_cameraX = x;
    m_cameraY = y;
}

void GameManager::enterScene(int sceneId) {
    m_currentSceneId = sceneId;
    SceneManager::getInstance().SetCurrentScene(sceneId);
}

void GameManager::AddItem(int itemId, int amount) {
    for (auto& item : m_inventory) {
        if (item.id == itemId) {
            item.amount += amount;
            if (item.amount > MAX_ITEM_AMOUNT) item.amount = MAX_ITEM_AMOUNT;
            return;
        }
    }
    InventoryItem newItem;
    newItem.id = itemId;
    newItem.amount = amount;
    if (newItem.amount > MAX_ITEM_AMOUNT) newItem.amount = MAX_ITEM_AMOUNT;
    m_inventory.push_back(newItem);
}

int GameManager::getItemAmount(int itemId) {
    for (const auto& item : m_inventory) {
        if (item.id == itemId) return item.amount;
    }
    return 0;
}

void GameManager::useItem(int itemId) {
    // Basic logic
    if (getItemAmount(itemId) > 0) {
        // Apply effect...
        // Decrease count
        for (auto it = m_inventory.begin(); it != m_inventory.end(); ++it) {
            if (it->id == itemId) {
                it->amount--;
                if (it->amount <= 0) {
                    m_inventory.erase(it);
                }
                break;
            }
        }
    }
}

void GameManager::EatOneItem(int roleNum, int itemId, int where) {
    if (roleNum < 0 || roleNum >= m_roles.size()) return;
    if (itemId < 0 || itemId >= m_items.size()) return;

    Role& role = m_roles[roleNum];
    Item& item = m_items[itemId];
    
    if (where == 0) {
        if (item.getEquipType() == 0) {
            role.setCurrentHP(std::min((int)role.getMaxHP(), role.getCurrentHP() + item.getAddCurrentHP()));
            role.setCurrentMP(std::min((int)role.getMaxMP(), role.getCurrentMP() + item.getAddCurrentMP()));
            
            role.setMaxHP(std::min(MAX_HP, role.getMaxHP() + item.getAddMaxHP()));
            role.setMaxMP(std::min(MAX_MP, role.getMaxMP() + item.getAddMaxMP()));
            
            role.setPhyPower(std::min(MAX_PHYSICAL_POWER, role.getPhyPower() + item.getAddPhyPower()));
            role.setPoision(std::max(0, role.getPoision() - item.getAddPoi()));
            
            role.setSpeed(std::min(100, role.getSpeed() + item.getAddSpeed()));
            role.setAttack(std::min(100, role.getAttack() + item.getAddAttack()));
            role.setDefence(std::min(100, role.getDefence() + item.getAddDefence()));
            
            role.setMedicine(std::min(100, role.getMedicine() + item.getAddMedcine()));
            role.setMedPoi(std::min(100, role.getMedPoi() + item.getAddMedPoi()));
            role.setUsePoi(std::min(100, role.getUsePoi() + item.getAddUsePoi()));
            role.setDefPoi(std::min(100, role.getDefPoi() + item.getAddDefPoi()));
            
            role.setFist(std::min(100, role.getFist() + item.getAddFist()));
            role.setSword(std::min(100, role.getSword() + item.getAddSword()));
            role.setKnife(std::min(100, role.getKnife() + item.getAddKnife()));
            role.setUnusual(std::min(100, role.getUnusual() + item.getAddUnusual()));
            role.setHidWeapon(std::min(100, role.getHidWeapon() + item.getAddHidWeapon()));
        }
    }
}

// Stubs for Getters needed by header
int GameManager::GetRoleMedcine(int roleNum, bool checkEquip) { return getRole(roleNum).getMedicine(); }
int GameManager::GetRoleMedPoi(int roleNum, bool checkEquip) { return getRole(roleNum).getMedPoi(); }
int GameManager::GetRoleUsePoi(int roleNum, bool checkEquip) { return getRole(roleNum).getUsePoi(); }
int GameManager::GetRoleDefPoi(int roleNum, bool checkEquip) { return getRole(roleNum).getDefPoi(); }
int GameManager::GetRoleFist(int roleNum, bool checkEquip) { return getRole(roleNum).getFist(); }
int GameManager::GetRoleSword(int roleNum, bool checkEquip) { return getRole(roleNum).getSword(); }
int GameManager::GetRoleKnife(int roleNum, bool checkEquip) { return getRole(roleNum).getKnife(); }
int GameManager::GetRoleUnusual(int roleNum, bool checkEquip) { return getRole(roleNum).getUnusual(); }
int GameManager::GetRoleHidWeapon(int roleNum, bool checkEquip) { return getRole(roleNum).getHidWeapon(); }

bool GameManager::GetEquipState(int roleIdx, int state) { return false; } // TODO
int GameManager::GetGongtiLevel(int roleIdx, int magicId) { return 0; } // TODO
bool GameManager::GetGongtiState(int roleIdx, int state) { return false; } // TODO
void GameManager::JoinParty(int roleId) { /* TODO */ }
void GameManager::LeaveParty(int roleId) { /* TODO */ }
void GameManager::Rest() { /* TODO */ }


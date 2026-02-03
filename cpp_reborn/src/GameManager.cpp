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
    // KYS loads initial data from "save/ranger.grp" (or similar) when num=0
    // But Pascal code shows:
    // if num = 0 then filename := 'ranger';
    // idx := fileopen(AppPath + 'save/ranger.idx', fmopenread);
    // grp := fileopen(AppPath + 'save/' + filename + '.grp', fmopenread);
    //
    // So for a new game, we should load from "save/ranger.grp".
    // Note: The file might be named "Ranger.grp" (case sensitive on Linux, but likely "ranger.grp" or "Ranger.grp" on Windows)
    
    std::string rolePath = savePrefix + "ranger.grp"; // Try lowercase first as per Pascal
    
    // Check if ranger.grp exists, if not try Ranger.grp
    // Actually FileLoader::loadFile might handle some path logic, but let's be explicit if possible.
    // However, the Pascal code also loads from "save/R1.grp" for slot 1.
    // The initial data seems to be in "ranger.grp".
    
    // Let's use FileLoader to load the whole file first, then parse it.
    // Unlike individual .grp files for Roles/Items (which don't seem to exist separately in this version),
    // KYS stores EVERYTHING in one big save file (ranger.grp / R1.grp).
    // Structure:
    // Header (InShip, Where, Mx, My... ~100 bytes?)
    // TeamList
    // Items
    // Roles
    // Items (Wait, RItem is separate from RItemList?)
    // Scenes
    // Magics
    // Shops
    
    // We need to match this structure to load data correctly!
    // Simply loading "role.grp" was wrong because that file doesn't exist.
    // We must load "ranger.grp" and seek to the correct offsets.
    // Offsets are stored in "ranger.idx".
    
    std::string idxPath = savePrefix + "ranger.idx";
    
    // Direct load (bypass FileLoader which enforces resource path)
    std::ifstream idxFile(idxPath, std::ios::binary | std::ios::ate);
    if (!idxFile) {
         std::cerr << "Failed to open " << idxPath << std::endl;
         return;
    }
    std::streamsize idxSize = idxFile.tellg();
    idxFile.seekg(0, std::ios::beg);
    std::vector<uint8_t> idxData(idxSize);
    idxFile.read((char*)idxData.data(), idxSize);
    
    if (idxData.size() < 24) { // At least 6 integers * 4 bytes
        std::cerr << "Failed to load ranger.idx or invalid size" << std::endl;
        return;
    }
    
    const int32_t* idxPtr = reinterpret_cast<const int32_t*>(idxData.data());
    int RoleOffset = idxPtr[0];
    int ItemOffset = idxPtr[1];
    int SceneOffset = idxPtr[2];
    int MagicOffset = idxPtr[3];
    int WeiShopOffset = idxPtr[4];
    int TotalLen = idxPtr[5];
    
    std::cout << "Ranger IDX Offsets: Role=" << RoleOffset 
              << ", Item=" << ItemOffset 
              << ", Scene=" << SceneOffset 
              << ", Magic=" << MagicOffset << std::endl;
              
    std::ifstream grpFile(rolePath, std::ios::binary | std::ios::ate);
    if (!grpFile) {
        // Try capitalized "Ranger.grp"
        rolePath = savePrefix + "Ranger.grp";
        grpFile.open(rolePath, std::ios::binary | std::ios::ate);
    }
    
    if (!grpFile) {
        std::cerr << "Failed to load ranger.grp" << std::endl;
        return;
    }
    
    std::streamsize grpSize = grpFile.tellg();
    grpFile.seekg(0, std::ios::beg);
    std::vector<uint8_t> saveBytes(grpSize);
    grpFile.read((char*)saveBytes.data(), grpSize);
    
    const uint8_t* dataPtr = saveBytes.data();
    
    // 0. Load Header (Global State)
    if (RoleOffset > 0) {
        // Assume Header is at the beginning
        // KYS Save Header structure (Partial, based on Pascal code):
        // Offset 0: InShip (2)
        // Offset 2: InSubMap (2) -> Current Scene ID
        // Offset 4: MainMapX (2)
        // Offset 6: MainMapY (2)
        // Offset 8: MainMapFace (2)
        
        if (RoleOffset >= 22) {
             m_currentSceneId = *(int16_t*)(dataPtr + 2);
             m_mainMapX = *(int16_t*)(dataPtr + 4);
             m_mainMapY = *(int16_t*)(dataPtr + 6);
             m_mainMapFace = *(int16_t*)(dataPtr + 8);
             
             // Sync SceneManager
             SceneManager::getInstance().SetCurrentScene(m_currentSceneId);
             
             // Check for invalid scene ID (0 is sometimes valid, but usually main menu or test)
             // If Scene ID is 0, it might be uninitialized or wrong.
             // But KYS Scene 0 is a valid scene (usually).
             
             // Fallback: If Scene ID is 0 and X/Y are 0, it's likely a bad header read.
             if (m_currentSceneId == 0 && m_mainMapX == 0 && m_mainMapY == 0) {
                 std::cerr << "WARNING: Loaded suspicious initial state (Scene 0, 0,0). Ranger.grp might be empty/invalid." << std::endl;
                 // Force fallback to known start if user is stuck?
                 // But user said "New Game enters Mongolian Tent" before.
                 // Maybe we should trust it unless it's strictly 0,0,0.
             }
             
             std::cout << "[loadData] Header Loaded: Scene=" << m_currentSceneId 
                       << " Pos=(" << m_mainMapX << "," << m_mainMapY << ")" 
                       << " Face=" << m_mainMapFace << std::endl;
                       
             // Load TeamList
             // Assuming TeamList starts at offset 22 (0x16)
             // KYS TeamList size: 6? 
             // Let's verify size.
             // If we assume TeamList is array[0..5] of integer (16-bit) -> 12 bytes.
             int16_t* teamSrc = (int16_t*)(dataPtr + 22);
             m_teamList.assign(MAX_TEAM_SIZE, -1); // Reset
             for(int i=0; i<MAX_TEAM_SIZE; ++i) {
                 if (22 + i*2 < RoleOffset) {
                     m_teamList[i] = teamSrc[i];
                 }
             }
             std::cout << "[loadData] TeamList: " << m_teamList[0] << ", " << m_teamList[1] << "..." << std::endl;
        }
    }
    
    // 1. Load Roles
    // Size = ItemOffset - RoleOffset
    if (ItemOffset > RoleOffset && ItemOffset <= saveBytes.size()) {
        int roleDataSize = ItemOffset - RoleOffset;
        const int bytesPerRole = static_cast<int>(ROLE_DATA_SIZE * sizeof(int16));
        int numRoles = (bytesPerRole > 0) ? (roleDataSize / bytesPerRole) : 0;
        m_roles.resize(numRoles);
        
        const int16_t* roleSrc = reinterpret_cast<const int16_t*>(dataPtr + RoleOffset);
        for (int i = 0; i < numRoles; ++i) {
            m_roles[i].loadFromBuffer(roleSrc + static_cast<size_t>(i) * ROLE_DATA_SIZE, ROLE_DATA_SIZE);
        }
        std::cout << "Loaded " << numRoles << " roles from ranger.grp" << std::endl;
    }
    
    // 2. Load Items
    // Size = SceneOffset - ItemOffset
    if (SceneOffset > ItemOffset && SceneOffset <= saveBytes.size()) {
        int itemDataSize = SceneOffset - ItemOffset;
        const int bytesPerItem = static_cast<int>(ITEM_DATA_SIZE * sizeof(int16));
        int numItems = (bytesPerItem > 0) ? (itemDataSize / bytesPerItem) : 0;
        m_items.resize(numItems);
        
        const int16_t* itemSrc = reinterpret_cast<const int16_t*>(dataPtr + ItemOffset);
        for (int i = 0; i < numItems; ++i) {
            m_items[i].loadFromBuffer(itemSrc + static_cast<size_t>(i) * ITEM_DATA_SIZE, ITEM_DATA_SIZE);
        }
        std::cout << "Loaded " << numItems << " items from ranger.grp" << std::endl;
    }
    
    // 3. Load Magics
    // Size = WeiShopOffset - MagicOffset
    if (WeiShopOffset > MagicOffset && WeiShopOffset <= saveBytes.size()) {
        int magicDataSize = WeiShopOffset - MagicOffset;
        const int bytesPerMagic = static_cast<int>(MAGIC_DATA_SIZE * sizeof(int16));
        int numMagics = (bytesPerMagic > 0) ? (magicDataSize / bytesPerMagic) : 0;
        m_magics.resize(numMagics);
        
        const int16_t* magicSrc = reinterpret_cast<const int16_t*>(dataPtr + MagicOffset);
        for (int i = 0; i < numMagics; ++i) {
            m_magics[i].loadFromBuffer(magicSrc + static_cast<size_t>(i) * MAGIC_DATA_SIZE, MAGIC_DATA_SIZE);
        }
        std::cout << "Loaded " << numMagics << " magics from ranger.grp" << std::endl;
    }
    
    // 4. Load Scenes
    // Size = MagicOffset - SceneOffset
    // NOTE: In Pascal: RScene: array of TScene;
    // TScene size is 26 * 2 = 52 bytes.
    if (MagicOffset > SceneOffset && MagicOffset <= saveBytes.size()) {
         int sceneDataSize = MagicOffset - SceneOffset;
         const int bytesPerScene = static_cast<int>(SCENE_DATA_SIZE * sizeof(int16));
         int numScenes = (bytesPerScene > 0) ? (sceneDataSize / bytesPerScene) : 0;
         std::vector<Scene> scenes(numScenes);
         
         const int16_t* sceneSrc = reinterpret_cast<const int16_t*>(dataPtr + SceneOffset);
         for (int i = 0; i < numScenes; ++i) {
             scenes[i].loadFromBuffer(sceneSrc + static_cast<size_t>(i) * SCENE_DATA_SIZE, SCENE_DATA_SIZE);
         }
         SceneManager::getInstance().SetScenes(scenes);
         std::cout << "Loaded " << numScenes << " scenes from ranger.grp. Data Size: " << sceneDataSize << " bytes." << std::endl;
    }
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

    // 1. Write Roles
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

    // 2. Write Items
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

    // 3. Write Scenes
    int sceneBytesToWrite = MagicOffset - SceneOffset;
    int sceneBytesWritten = 0;
    const auto& scenes = SceneManager::getInstance().getScenes();
    for (const auto& scene : scenes) {
        if (sceneBytesWritten + SCENE_DATA_SIZE * 2 <= sceneBytesToWrite) {
            grpFile.write((char*)scene.getRawData(), SCENE_DATA_SIZE * 2);
            sceneBytesWritten += SCENE_DATA_SIZE * 2;
        } else break;
    }
    while (sceneBytesWritten < sceneBytesToWrite) {
        char zero = 0;
        grpFile.write(&zero, 1);
        sceneBytesWritten++;
    }

    // 4. Write Magics
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

    // 5. Write Shops (WeiShop)
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
    
    std::cout << "[LoadGame] Offsets: Role=" << RoleOffset << " Item=" << ItemOffset 
              << " Scene=" << SceneOffset << " Magic=" << MagicOffset << std::endl;

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
    std::cout << "[LoadGame] Opened " << grpPath << std::endl;

    auto read16 = [&](int16_t& val) {
        grpFile.read((char*)&val, 2);
    };

    read16(m_inShip);
    int16_t tempWhere;
    read16(tempWhere);
    if (tempWhere < 0) tempWhere = 0;
    m_currentSceneId = tempWhere;
    // Don't call SetCurrentScene here, it triggers RefreshEventLayer which fails because data isn't loaded yet.
    // SceneManager::getInstance().SetCurrentScene(m_currentSceneId); 
    // Just set the ID directly in SceneManager if needed, or wait until end.
    SceneManager::getInstance().SetCurrentScene(m_currentSceneId); // Keep it but we know it fails refresh

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

    std::cout << "[LoadGame] Header loaded. Scene=" << m_currentSceneId << " Pos=(" << m_mainMapX << "," << m_mainMapY << ")" << std::endl;

    m_teamList.resize(MAX_TEAM_SIZE);
    for(int i=0; i<MAX_TEAM_SIZE; ++i) {
        read16((int16_t&)m_teamList[i]);
    }

    m_inventory.resize(MAX_ITEM_AMOUNT);
    for(int i=0; i<MAX_ITEM_AMOUNT; ++i) {
        read16(m_inventory[i].id);
        read16(m_inventory[i].amount);
    }
    
    std::cout << "[LoadGame] Loading Roles..." << std::endl;
    grpFile.seekg(RoleOffset, std::ios::beg);
    int roleSize = (ItemOffset - RoleOffset) / (ROLE_DATA_SIZE * 2);
    if (roleSize < 0 || roleSize > 10000) {
        std::cerr << "[LoadGame] ERROR: Invalid roleSize " << roleSize << std::endl;
        return;
    }
    m_roles.resize(roleSize);
    for(int i=0; i<roleSize; ++i) {
        std::vector<int16_t> buffer(ROLE_DATA_SIZE);
        grpFile.read((char*)buffer.data(), ROLE_DATA_SIZE * 2);
        m_roles[i].setDataVector(buffer);
    }

    std::cout << "[LoadGame] Loading Items..." << std::endl;
    grpFile.seekg(ItemOffset, std::ios::beg);
    int itemSize = (SceneOffset - ItemOffset) / (ITEM_DATA_SIZE * 2);
    if (itemSize < 0 || itemSize > 10000) {
        std::cerr << "[LoadGame] ERROR: Invalid itemSize " << itemSize << std::endl;
        return;
    }
    m_items.resize(itemSize);
    for(int i=0; i<itemSize; ++i) {
        std::vector<int16_t> buffer(ITEM_DATA_SIZE);
        grpFile.read((char*)buffer.data(), ITEM_DATA_SIZE * 2);
        m_items[i].setDataVector(buffer);
    }

    std::cout << "[LoadGame] Loading Scenes..." << std::endl;
    grpFile.seekg(SceneOffset, std::ios::beg);
    int sceneSize = (MagicOffset - SceneOffset) / (SCENE_DATA_SIZE * 2);
    if (sceneSize < 0 || sceneSize > 10000) {
         std::cerr << "[LoadGame] ERROR: Invalid sceneSize " << sceneSize << std::endl;
         return;
    }
    std::vector<Scene> scenes(sceneSize);
    for(int i=0; i<sceneSize; ++i) {
        std::vector<int16_t> buffer(SCENE_DATA_SIZE);
        grpFile.read((char*)buffer.data(), SCENE_DATA_SIZE * 2);
        scenes[i].setDataVector(buffer);
    }
    SceneManager::getInstance().SetScenes(scenes);

    std::cout << "[LoadGame] Loading Magics..." << std::endl;
    grpFile.seekg(MagicOffset, std::ios::beg);
    int magicSize = (WeiShopOffset - MagicOffset) / (MAGIC_DATA_SIZE * 2);
    if (magicSize < 0 || magicSize > 10000) {
        std::cerr << "[LoadGame] ERROR: Invalid magicSize " << magicSize << std::endl;
        return;
    }
    m_magics.resize(magicSize);
    for(int i=0; i<magicSize; ++i) {
        std::vector<int16_t> buffer(MAGIC_DATA_SIZE);
        grpFile.read((char*)buffer.data(), MAGIC_DATA_SIZE * 2);
        m_magics[i].setDataVector(buffer);
    }

    grpFile.close();
    std::cout << "[LoadGame] ranger.grp loaded successfully." << std::endl;

    std::string sFilename = (slot == 0) ? "allsin.grp" : "S" + std::to_string(slot) + ".grp";
    std::string dFilename = (slot == 0) ? "alldef.grp" : "D" + std::to_string(slot) + ".grp";
    
    std::cout << "[LoadGame] Loading Maps: " << sFilename << " & " << dFilename << std::endl;

    if (!SceneManager::getInstance().LoadMapData(m_savePath + sFilename)) {
        if (slot == 0) SceneManager::getInstance().LoadMapData(m_savePath + "allsin.grp");
    }
    if (!SceneManager::getInstance().LoadEventData(m_savePath + dFilename)) {
        if (slot == 0) SceneManager::getInstance().LoadEventData(m_savePath + "alldef.grp");
    }
    
    // Force Refresh Layer 3 after everything is loaded
    if (m_currentSceneId >= 0) {
        SceneManager::getInstance().RefreshEventLayer(m_currentSceneId);
    }
    
    std::cout << "Game Loaded from Slot " << slot << std::endl;
}

void GameManager::InitNewGame() {
    // KYS New Game: Load initial state from ranger.grp
    // User feedback indicates that ranger.grp acts as the "New Game Save".
    
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
    // 修正：新游戏强制从场景0（圣堂）开始
    // ranger.grp 的头部可能保存的是大地图状态(-1)，这对新游戏是不正确的。
    // 实际的开场剧情由场景0的事件101处理。
    m_currentSceneId = 0; 
    
    // Updated based on user feedback: Correct start position in Scene 0 is (38, 38)
    m_mainMapX = 38;
    m_mainMapY = 38;
    
    // Set default world map position for when player exits the initial scene
    // This position should be near the entrance of Scene 0 (Temple) on the world map.
    // Based on typical KYS map, Temple is around (228, 228) or similar.
    // However, if we don't know, we can look it up from Scene 0 data.
    if (SceneManager::getInstance().GetSceneCount() > 0) {
        Scene* scene0 = SceneManager::getInstance().GetScene(0);
        if (scene0) {
            m_savedWorldX = scene0->getMainEntranceX1();
            m_savedWorldY = scene0->getMainEntranceY1();
        }
    }
    // Fallback if 0
    if (m_savedWorldX == 0 && m_savedWorldY == 0) {
        m_savedWorldX = 194; // Approximate
        m_savedWorldY = 267;
    }
    
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
    role.setMedcine(dis(gen));
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
                    if (m_titleMenuSelection == 0) {// 新游戏
                        m_currentState = GameState::CharacterCreation;
                        //RandomizeRoleStats(getRole(0));
                        //m_characterCreationNameUtf8 = TextManager::getInstance().nameToUtf8(getRole(0).getName());
                        //if (m_characterCreationNameUtf8.empty()) {
                            m_characterCreationNameUtf8 = "金先生";
                       // }
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
                            case 0: frontX++; break;
                            case 1: frontX--; break;
                            case 2: frontY--; break;
                            case 3: frontY++; break;
                        }
                        if (m_currentSceneId >= 0) {
                            EventManager::getInstance().CheckEvent(m_currentSceneId, frontX, frontY, true);
                        }
                    }
                    break;
                    
                case SDLK_C: 
                    if (!m_teamList.empty()) {
                        UIManager::getInstance().ShowStatus(m_teamList[0]);
                    }
                    break;

                case SDLK_ESCAPE:
                    if (m_currentSceneId >= 0) {
                        SceneManager::getInstance().DrawScene(m_renderer, m_cameraX, m_cameraY);
                        RenderScreenTo(m_renderer);
                        UIManager::getInstance().ShowMenu();
                    }
                    break;
            }
            
            if (dx != 0 || dy != 0) {
                int nextX = m_mainMapX + dx;
                int nextY = m_mainMapY + dy;

                if (m_currentSceneId >= 0) {
                    if (SceneManager::getInstance().CanWalk(nextX, nextY)) {
                        m_mainMapX = nextX;
                        m_mainMapY = nextY;
                        m_cameraX = m_mainMapX;
                        m_cameraY = m_mainMapY;
                        updateWalkFrame();
                        EventManager::getInstance().CheckEvent(m_currentSceneId, m_mainMapX, m_mainMapY, false);

                        // Check Scene Exit
                        // Pascal: if (((sx = RScene[CurScene].ExitX[0]) and (sy = RScene[CurScene].ExitY[0])) ...
                        Scene* scene = SceneManager::getInstance().GetScene(m_currentSceneId);
                        if (scene) {
                            bool atExit = false;
                            for (int i = 0; i < 3; ++i) {
                                int exitX = scene->getExitX(i);
                                int exitY = scene->getExitY(i);
                                if (exitX > 0 && exitY > 0 && m_mainMapX == exitX && m_mainMapY == exitY) {
                                    atExit = true;
                                    break;
                                }
                            }
                            
                            if (atExit) {
                                UIManager::getInstance().FadeScreen(false);
                                
                                m_currentSceneId = -1; // Switch to World Map
                                SceneManager::getInstance().SetCurrentScene(-1);
                                SceneManager::getInstance().ResetEntrance();
                                
                                // Restore World Map Position
                                m_mainMapX = m_savedWorldX;
                                m_mainMapY = m_savedWorldY;
                                m_cameraX = m_mainMapX;
                                m_cameraY = m_mainMapY;
                                
                                UIManager::getInstance().FadeScreen(true);
                            }
                        }
                    }
                } else {
                    // 大地图移动与入口检测
                    if (CanWalkWorld(nextX, nextY)) {
                        m_mainMapX = nextX;
                        m_mainMapY = nextY;
                        m_cameraX = m_mainMapX;
                        m_cameraY = m_mainMapY;
                        updateWalkFrame();
                        CheckWorldEntrance();
                    }
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

bool GameManager::CanWalkWorld(int x, int y) {
    if (x < 0 || x >= 480 || y < 0 || y >= 480) return false;

    SceneManager& sm = SceneManager::getInstance();
    int16_t buildx = sm.GetWorldBuildX(x, y);
    int16_t surface = sm.GetWorldSurface(x, y);
    int16_t earth = sm.GetWorldEarth(x, y);

    bool canwalk = (buildx == 0);

    if (x <= 0 || x >= 479 || y <= 0 || y >= 479 ||
        (surface >= 1692 && surface <= 1700)) {
        canwalk = false;
    }

    if (earth == 838 || (earth >= 612 && earth <= 670)) {
        canwalk = false;
    }

    if ((earth >= 358 && earth <= 362) ||
        (earth >= 506 && earth <= 670) ||
        (earth >= 1016 && earth <= 1022)) {
        if (m_inShip == 1) {
            if (earth == 838 || (earth >= 612 && earth <= 670)) {
                canwalk = false;
            } else if (surface >= 1746 && surface <= 1788) {
                canwalk = false;
            } else {
                canwalk = true;
            }
        } else if (x == m_shipY && y == m_shipX) {
            canwalk = true;
            m_inShip = 1;
        } else {
            canwalk = false;
        }
    } else {
        if (m_inShip == 1) {
            m_shipY = static_cast<int16_t>(m_mainMapX);
            m_shipX = static_cast<int16_t>(m_mainMapY);
            m_shipFace = static_cast<int16_t>(m_mainMapFace);
        }
        m_inShip = 0;
    }

    int surfaceHalf = surface / 2;
    if ((surfaceHalf >= 863 && surfaceHalf <= 872) ||
        (surfaceHalf >= 852 && surfaceHalf <= 854) ||
        (surfaceHalf >= 858 && surfaceHalf <= 860)) {
        canwalk = true;
    }

    return canwalk;
}

bool GameManager::CheckWorldEntrance() {
    int x = m_mainMapX;
    int y = m_mainMapY;

    switch (m_mainMapFace) {
        case 0: x += 1; break;
        case 1: x -= 1; break;
        case 2: y -= 1; break;
        case 3: y += 1; break;
    }

    int16_t snum = SceneManager::getInstance().GetEntrance(x, y);
    if (snum < 0) return false;

    Scene& scene = getScene(snum);
    bool canEntrance = false;

    int16_t enCond = scene.getEnCondition();
    if (enCond == 0) {
        canEntrance = true;
    } else if (enCond == 2) {
        for (int roleId : m_teamList) {
            if (roleId < 0) continue;
            Role& role = getRole(roleId);
            if (role.getSpeed() >= 70) {
                canEntrance = true;
                break;
            }
        }
    }

    if (!canEntrance) return false;

    UIManager::getInstance().FadeScreen(false);

    // Save world coordinates before entering scene
    m_savedWorldX = m_mainMapX;
    m_savedWorldY = m_mainMapY;

    int16_t entranceX = scene.getEntranceX();
    int16_t entranceY = scene.getEntranceY();

    enterScene(snum);
    m_subMapFace = static_cast<int16_t>(m_mainMapFace);
    m_mainMapFace = 3 - m_mainMapFace;
    resetWalkFrame();

    setMainMapPosition(entranceX, entranceY);

    return true;
}

void GameManager::UpdateSystemMenu() {
    UIManager::getInstance().ShowMenu();
    m_currentState = GameState::Roaming;
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
            
            role.setMedcine(std::min(100, role.getMedcine() + item.getAddMedcine()));
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
int GameManager::GetRoleMedcine(int roleNum, bool checkEquip) { return getRole(roleNum).getMedcine(); }
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

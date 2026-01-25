#include "SceneManager.h"
#include "FileLoader.h"
#include "GraphicsUtils.h"
#include "GameManager.h"
#include "TextManager.h"
#include <iostream>
#include <cstring>
#include <algorithm>

SceneManager& SceneManager::getInstance() {
    static SceneManager instance;
    return instance;
}

SceneManager::SceneManager() : m_currentSceneId(0) {}

bool SceneManager::Init() {
    // Load resources
    if (!LoadResources()) {
        std::cerr << "Warning: Failed to load tile resources (smp/sdx)" << std::endl;
        // Don't fail completely, maybe resources are missing but we can still load map logic
    }
    // 5. Load World Map Resources (wmp/wdx)
    m_wPicData = FileLoader::loadFile("resource/wmp");
    auto wIdxBytes = FileLoader::loadFile("resource/wdx");
    
    if (!m_wPicData.empty() && !wIdxBytes.empty()) {
        size_t count = wIdxBytes.size() / 4;
        m_wIdxData.resize(count);
        memcpy(m_wIdxData.data(), wIdxBytes.data(), wIdxBytes.size());
        std::cout << "[SceneManager] Loaded wmp/wdx. Pic Data: " << m_wPicData.size() << " bytes, Idx Count: " << count << std::endl;
    } else {
        std::cerr << "[SceneManager] Failed to load wmp/wdx" << std::endl;
    }
    
    // 6. Load World Map Layout (EARTH.002)
    LoadWorldMap();

    return true;
}

bool SceneManager::LoadWorldMap() {
    auto data = FileLoader::loadFile("EARTH.002");
    if (data.empty()) {
        // Try fallback names
        data = FileLoader::loadFile("resource/EARTH.002");
        if (data.empty()) {
             std::cerr << "Failed to load EARTH.002" << std::endl;
             return false;
        }
    }
    
    // Size check
    // 480 * 480 * 2 (int16) = 460800 bytes per layer.
    // Usually 1 layer? Or 2?
    // KYS World Map is typically 1 layer of ground.
    // Buildings are separate? No, World Map buildings are often part of the map or events.
    // Let's assume 1 layer for now.
    
    m_worldMapData.resize(data.size() / 2);
    memcpy(m_worldMapData.data(), data.data(), data.size());
    std::cout << "Loaded World Map. Size: " << m_worldMapData.size() << " tiles." << std::endl;
    
    return true;
}

bool SceneManager::LoadResources() {


    // 1. Load Tile Graphics (smp/sdx)
    m_sPicData = FileLoader::loadFile("resource/smp");
    auto idxBytes = FileLoader::loadFile("resource/sdx");
    
    if (!m_sPicData.empty() && !idxBytes.empty()) {
        size_t count = idxBytes.size() / 4;
        m_sIdxData.resize(count);
        memcpy(m_sIdxData.data(), idxBytes.data(), idxBytes.size());
    } else {
        std::cerr << "Failed to load smp/sdx" << std::endl;
        return false; 
    }
    
    // 2. Load Sprite Graphics (mmap.grp/mmap.idx)
    m_mmapPicData = FileLoader::loadFile("resource/mmap.grp");
    auto mmapIdxBytes = FileLoader::loadFile("resource/mmap.idx");
    
    if (!m_mmapPicData.empty() && !mmapIdxBytes.empty()) {
        size_t count = mmapIdxBytes.size() / 4;
        m_mmapIdxData.resize(count);
        memcpy(m_mmapIdxData.data(), mmapIdxBytes.data(), mmapIdxBytes.size());
        std::cout << "Loaded mmap.grp with " << count << " sprites." << std::endl;
    } else {
        std::cerr << "Failed to load mmap.grp/mmap.idx" << std::endl;
    }

    // 3. Load Cloud Graphics (cloud.grp/cloud.idx)
    m_cloudPicData = FileLoader::loadFile("resource/cloud.grp");
    auto cloudIdxBytes = FileLoader::loadFile("resource/cloud.idx");
    
    if (!m_cloudPicData.empty() && !cloudIdxBytes.empty()) {
        size_t count = cloudIdxBytes.size() / 4;
        m_cloudIdxData.resize(count);
        memcpy(m_cloudIdxData.data(), cloudIdxBytes.data(), cloudIdxBytes.size());
    } else {
        std::cerr << "Failed to load cloud.grp/cloud.idx" << std::endl;
    }

    // 4. Load Palette (MMAP.COL)
    std::string palPath = FileLoader::getResourcePath("resource/MMAP.COL");
    GraphicsUtils::loadPalette(palPath);
    
    return true;
}

bool SceneManager::LoadEventData(const std::string& path) {
    auto data = FileLoader::loadFile(path);
    if (data.empty()) return false;

    // DData size per scene: 200 events * 11 ints * 2 bytes = 4400 bytes
    size_t sceneSize = 200 * 11 * 2;
    if (data.size() % sceneSize != 0) {
        std::cerr << "Warning: Event data size mismatch. Expected multiple of " << sceneSize << ", got " << data.size() << std::endl;
    }

    size_t count = data.size() / sceneSize;
    m_eventData.resize(count);
    
    for (size_t i = 0; i < count; ++i) {
        memcpy(m_eventData[i].data, data.data() + i * sceneSize, sceneSize);
    }
    
    // Debug: Check Scene 0, Event 3 (Kong Pili?)
    if (count > 0) {
        std::cout << "[LoadEventData] Checking Scene 0, Event 3:" << std::endl;
        for(int k=0; k<11; ++k) {
            std::cout << "  Index " << k << ": " << m_eventData[0].data[3][k] << std::endl;
        }
        // Index 5 is supposed to be Pic. If it's 0, that's why it's invisible.
    }
    
    std::cout << "Loaded " << count << " scenes event data from " << path << std::endl;
    return true;
}

bool SceneManager::SaveEventData(const std::string& path) {
    if (m_eventData.empty()) return false;
    
    // DData size per scene: 200 events * 11 ints * 2 bytes = 4400 bytes
    size_t sceneSize = 200 * 11 * 2;
    size_t totalSize = m_eventData.size() * sceneSize;
    
    std::vector<uint8_t> buffer(totalSize);
    for (size_t i = 0; i < m_eventData.size(); ++i) {
        memcpy(buffer.data() + i * sceneSize, m_eventData[i].data, sceneSize);
    }
    
    return FileLoader::saveFile(path, buffer.data(), buffer.size());
}

bool SceneManager::LoadMapData(const std::string& path) {
    auto data = FileLoader::loadFile(path);
    if (data.empty()) return false;
    
    // SData size per scene: 6 layers * 64 * 64 tiles * 2 bytes/tile
    size_t sceneSize = SCENE_LAYERS * SCENE_MAP_SIZE * SCENE_MAP_SIZE * 2;
    if (data.size() % sceneSize != 0) {
        std::cerr << "Warning: Map data size mismatch. Expected multiple of " << sceneSize << ", got " << data.size() << std::endl;
    }
    
    m_mapData.resize(data.size() / 2);
    memcpy(m_mapData.data(), data.data(), data.size());
    
    return true;
}

bool SceneManager::SaveMapData(const std::string& path) {
    if (m_mapData.empty()) return false;
    
    // Convert int16 vector to bytes
    size_t byteSize = m_mapData.size() * 2;
    return FileLoader::saveFile(path, m_mapData.data(), byteSize);
}

void SceneManager::SetScenes(const std::vector<Scene>& scenes) {
    m_scenes = scenes;
    std::cout << "SceneManager: Set " << m_scenes.size() << " scenes." << std::endl;
}

void SceneManager::SetCurrentScene(int sceneId) {
    m_currentSceneId = sceneId;
}

Scene* SceneManager::GetScene(int sceneId) {
    if (sceneId < 0 || sceneId >= m_scenes.size()) return nullptr;
    return &m_scenes[sceneId];
}

int16_t SceneManager::GetSceneTile(int sceneId, int layer, int x, int y) const {
    if (sceneId < 0 || layer < 0 || layer >= SCENE_LAYERS || x < 0 || x >= SCENE_MAP_SIZE || y < 0 || y >= SCENE_MAP_SIZE) {
        return -1;
    }
    
    size_t sceneOffset = sceneId * (SCENE_LAYERS * SCENE_MAP_SIZE * SCENE_MAP_SIZE);
    size_t layerOffset = layer * (SCENE_MAP_SIZE * SCENE_MAP_SIZE);
    // Transposed access: x * 64 + y
    size_t tileOffset = x * SCENE_MAP_SIZE + y;
    
    size_t index = sceneOffset + layerOffset + tileOffset;
    if (index >= m_mapData.size()) return -1;
    
    return m_mapData[index];
}

void SceneManager::SetSceneTile(int sceneId, int layer, int x, int y, int16_t value) {
    if (sceneId < 0 || layer < 0 || layer >= SCENE_LAYERS || x < 0 || x >= SCENE_MAP_SIZE || y < 0 || y >= SCENE_MAP_SIZE) {
        return;
    }
    
    size_t sceneOffset = sceneId * (SCENE_LAYERS * SCENE_MAP_SIZE * SCENE_MAP_SIZE);
    size_t layerOffset = layer * (SCENE_MAP_SIZE * SCENE_MAP_SIZE);
    // Transposed access: x * 64 + y
    size_t tileOffset = x * SCENE_MAP_SIZE + y;
    
    size_t index = sceneOffset + layerOffset + tileOffset;
    if (index >= m_mapData.size()) return;
    
    m_mapData[index] = value;
}

int16_t SceneManager::GetEventData(int sceneId, int eventId, int index) const {
    if (sceneId < 0 || sceneId >= m_eventData.size()) return 0;
    if (eventId < 0 || eventId >= 200) return 0;
    if (index < 0 || index >= 11) return 0;
    return m_eventData[sceneId].data[eventId][index];
}

void SceneManager::SetEventData(int sceneId, int eventId, int index, int16_t value) {
    if (sceneId < 0 || sceneId >= m_eventData.size()) return;
    if (eventId < 0 || eventId >= 200) return;
    if (index < 0 || index >= 11) return;
    m_eventData[sceneId].data[eventId][index] = value;
}

void SceneManager::GetPositionOnScreen(int mapX, int mapY, int centerX, int centerY, int& outX, int& outY) {
    // Standard screen center
    const int SCREEN_CENTER_X = 320; 
    const int SCREEN_CENTER_Y = 240; 
    
    outX = -(mapX - centerX) * 18 + (mapY - centerY) * 18 + SCREEN_CENTER_X;
    outY = (mapX - centerX) * 9 + (mapY - centerY) * 9 + SCREEN_CENTER_Y;
}

bool SceneManager::CanWalk(int x, int y) {
    if (x < 0 || x >= SCENE_MAP_SIZE || y < 0 || y >= SCENE_MAP_SIZE) return false;
    
    // Layer 1 check (Building/Obstacle)
    int16_t tile1 = GetSceneTile(m_currentSceneId, 1, x, y);
    
    // Logic from KYS:
    // If tile1 > 0, it's an object/building.
    // However, some objects are walkable?
    // In original code, it checks if tile1 is 0.
    // If tile1 != 0, it is blocked.
    // UNLESS it's a special "overhead" tile which is drawn but walkable?
    // But usually Layer 1 is collision layer.
    
    if (tile1 != 0) return false;
    
    // Layer 0 (Ground) checks
    int16_t tile0 = GetSceneTile(m_currentSceneId, 0, x, y);
    // Water checks (simplified)
    // 358-362, 522, 1022, 1324-1330, 1348 are water/unwalkable
    if ((tile0 >= 358 && tile0 <= 362) || tile0 == 522 || tile0 == 1022 || 
        (tile0 >= 1324 && tile0 <= 1330) || tile0 == 1348) {
        return false;
    }
    
    return true;
}

void SceneManager::Update(uint32_t ticks) {
    // Water effect: Change palette every 200ms
    if (ticks - m_lastWaterUpdate > 200) {
        GraphicsUtils::ChangeCol(ticks);
        m_lastWaterUpdate = ticks;
    }
    
    // Cloud movement: every 40ms
    if (ticks - m_lastCloudUpdate > 40) {
        if (m_clouds.empty()) {
            m_clouds.resize(20); 
            for (auto& cloud : m_clouds) {
                cloud.x = rand() % 17280;
                cloud.y = rand() % 8640;
                cloud.speedX = 1 + rand() % 3;
                cloud.speedY = 0;
                cloud.picNum = rand() % 9;
                cloud.alpha = 25 + rand() % 50;
                cloud.shadow = 0;
            }
        }
        
        for (auto& cloud : m_clouds) {
            cloud.x += cloud.speedX;
            cloud.y += cloud.speedY;
            
            if (cloud.x > 17279) cloud.x = 0;
            if (cloud.y > 8639) cloud.y = 0;
        }
        
        m_lastCloudUpdate = ticks;
    }
}

void SceneManager::UpdateEventPosition(int sceneId, int eventId, int oldX, int oldY, int newX, int newY) {
    // Check if the old position actually contains this event
    // This prevents clearing another event if multiple events overlap (though unlikely in Layer 3)
    if (GetSceneTile(sceneId, 3, oldX, oldY) == eventId) {
        SetSceneTile(sceneId, 3, oldX, oldY, -1); // -1 = No Event
    }
    
    // Set new position
    if (newX >= 0 && newY >= 0) {
        SetSceneTile(sceneId, 3, newX, newY, eventId);
    }
}

void SceneManager::DrawClouds(SDL_Renderer* renderer, int centerX, int centerY) {
    if (m_cloudPicData.empty()) return;
    
    SDL_Surface* screen = GameManager::getInstance().getScreenSurface();
    if (!screen) return;
    
    // Draw clouds based on their position
    // Simple wrapping logic for screen coverage
    for (const auto& cloud : m_clouds) {
        // Map large world coords to screen
        // Just for visual effect:
        int sx = (cloud.x / 10) % (640 + 200) - 100;
        int sy = (cloud.y / 10) % (480 + 200) - 100;
        
        // Draw if within screen bounds (roughly)
        if (sx > -200 && sx < 640 && sy > -200 && sy < 480) {
            // Draw using RLE
            // Note: This draws opaque clouds. Alpha blending would require a custom blender.
            // For now, satisfy the "draw" requirement.
            if (cloud.picNum >= 0 && cloud.picNum < m_cloudIdxData.size()) {
                int offset = m_cloudIdxData[cloud.picNum];
                if (offset > 0 && offset < m_cloudPicData.size()) {
                     GraphicsUtils::DrawRLE8(screen, sx, sy, &m_cloudPicData[offset], m_cloudPicData.size() - offset);
                }
            }
        }
    }
}

void SceneManager::DrawWorldMap(SDL_Renderer* renderer, int centerX, int centerY) {
    if (m_wIdxData.empty() || m_wPicData.empty() || m_worldMapData.empty()) {
        static bool loggedEmpty = false;
        if (!loggedEmpty) {
            std::cerr << "[DrawWorldMap] Resources empty! Idx: " << m_wIdxData.size() << " Pic: " << m_wPicData.size() << " Map: " << m_worldMapData.size() << std::endl;
            loggedEmpty = true;
        }
        return;
    }

    int px, py;
    GameManager::getInstance().getMainMapPosition(px, py);
    
    // Debug: Log center tile
    static int lastCenterX = -1, lastCenterY = -1;
    if (centerX != lastCenterX || centerY != lastCenterY) {
         int tileIndex = centerY * 480 + centerX;
         if (tileIndex >= 0 && tileIndex < m_worldMapData.size()) {
             std::cout << "[DrawWorldMap] Center (" << centerX << "," << centerY << ") Tile: " << m_worldMapData[tileIndex] << std::endl;
         }
         lastCenterX = centerX;
         lastCenterY = centerY;
    }

    // Draw Map
    // Visible range
    int range = 20; // 20 tiles radius
    
    for (int i1 = centerX - range; i1 <= centerX + range; ++i1) {
        for (int i2 = centerY - range; i2 <= centerY + range; ++i2) {
            // Wrap coordinates? World Map wraps in KYS.
            // 480x480
            int mapX = (i1 + 480) % 480;
            int mapY = (i2 + 480) % 480;
            
            int tileIndex = mapY * 480 + mapX; // Transposed? Or X*480+Y?
            // KYS usually: Y * Width + X ? Or X * Height + Y?
            // Let's try standard Y * Width + X first.
            
            if (tileIndex < 0 || tileIndex >= m_worldMapData.size()) continue;
            
            int16_t tile = m_worldMapData[tileIndex];
            
            int x, y;
            GetPositionOnScreen(i1, i2, centerX, centerY, x, y);
            
            // Culling
            if (x < -100 || x > 740 || y < -100 || y > 580) continue; 
            
            // Draw Tile using wmp
            if (tile > 0) {
                // tile / 2 ?
                int picIndex = (tile / 2);
                if (picIndex < m_wIdxData.size()) {
                    int offset = m_wIdxData[picIndex];
                    if (offset > 0 && offset < m_wPicData.size()) {
                        GraphicsUtils::DrawRLE8(GameManager::getInstance().getScreenSurface(), x, y, &m_wPicData[offset], m_wPicData.size() - offset);
                    }
                }
            }
        }
    }
    
    // Draw Player
    int screenX, screenY;
    GetPositionOnScreen(centerX, centerY, centerX, centerY, screenX, screenY);
    
    int face = GameManager::getInstance().getMainMapFace();
    int spriteFace = 0;
    switch(face) {
         case 0: spriteFace = 3; break; // Down
         case 1: spriteFace = 0; break; // Up
         case 2: spriteFace = 2; break; // Left
         case 3: spriteFace = 1; break; // Right
    }
    int frame = GameManager::getInstance().getWalkFrame();
    int playerPic = 2501 + spriteFace * 7 + frame;
    
    // Draw Player using mmap (or smp?)
    // Usually on World Map, player uses same sprite.
    DrawSmpSprite(renderer, playerPic, screenX, screenY, 0);
    
    // Draw Clouds
    DrawClouds(renderer, centerX, centerY);
}

void SceneManager::DrawScene(SDL_Renderer* renderer, int centerX, int centerY) {
    // If Scene is -1 (World Map), delegate
    if (m_currentSceneId == -1) {
        DrawWorldMap(renderer, centerX, centerY);
        return;
    }

    // Safety check: if resources failed to load, don't draw
    if (m_sIdxData.empty() || m_sPicData.empty()) {
        static bool logged = false;
        if (!logged) {
            std::cerr << "[DrawScene] CRITICAL: Tile resources (smp/sdx) are empty! Cannot draw scene." << std::endl;
            logged = true;
        }
        return;
    }
    
    // Check if Map Data is loaded
    if (m_mapData.empty()) {
        static bool loggedMap = false;
        if (!loggedMap) {
            std::cerr << "[DrawScene] CRITICAL: Map Data (allsin.grp) is empty! Cannot draw scene." << std::endl;
            loggedMap = true;
        }
        return; // Nothing to draw
    }

    // Optimization: Pre-calculate Sprite Map
    // Map: [x][y] -> Sprite Index (Pic)
    // Initialize with -1 (No Sprite)
    // Using a flat vector or stack array? 64x64 is small (4KB).
    static int16_t spriteMap[SCENE_MAP_SIZE][SCENE_MAP_SIZE];
    for(int i=0; i<SCENE_MAP_SIZE; ++i)
        for(int j=0; j<SCENE_MAP_SIZE; ++j)
            spriteMap[i][j] = -1;
            
    // Fill with active events - COMMENTED OUT as it conflicts with Layer 3 logic and uses potentially wrong index (3 vs 5)
    /*
    if (m_currentSceneId >= 0 && m_currentSceneId < m_eventData.size()) {
        const auto& events = m_eventData[m_currentSceneId].data;
        for (int e = 0; e < 200; ++e) {
            // Index 0: Active/Type? Usually > 0 means active.
            if (events[e][0] != 0) {
                int ex = events[e][1];
                int ey = events[e][2];
                int pic = events[e][3];
                
                if (ex >= 0 && ex < SCENE_MAP_SIZE && ey >= 0 && ey < SCENE_MAP_SIZE) {
                    spriteMap[ex][ey] = pic;
                }
            }
        }
    }
    */
    
    // Add Player Sprite to Map
    // Logic: If Event 0 is active (has a sprite), it represents the player in cutscenes.
    // In this case, we hide the default player sprite to avoid duplication/ghosting.
    bool hidePlayer = false;
    if (m_currentSceneId >= 0 && m_currentSceneId < m_eventData.size()) {
        if (m_eventData[m_currentSceneId].data[0][5] > 0) { // Event 0, Index 5 (Pic)
            hidePlayer = true;
        }
    }

    int px, py;
    GameManager::getInstance().getMainMapPosition(px, py);
    if (!hidePlayer && px >= 0 && px < SCENE_MAP_SIZE && py >= 0 && py < SCENE_MAP_SIZE) {
        // Player sprite index
        // Base: 2501 (Protagonist)
        // Face Mapping:
        // 0 = South (Down) -> 0
        // 1 = North (Up) -> 2
        // 2 = West (Left) -> 3
        // 3 = East (Right) -> 1
        
        int face = GameManager::getInstance().getMainMapFace();
        int spriteFace = 0;
        // KYS ISO View Mapping:
         // 0 = South (Down) -> 0
         // 1 = North (Up) -> 1
         // 2 = West (Left) -> 2
         // 3 = East (Right) -> 3
         
         // Final Fine-Tuning based on feedback:
         // Left (2) -> Sprite 2 (Correct)
         // Down (0) -> Sprite 3 (Correct)
         // Right (3) -> Sprite 0 (Wrong)
         // Up (1) -> Sprite 1 (Wrong)
         
         // If Right(3) is not 0, and can't be 2 or 3 (taken).
         // Then Right(3) MUST be Sprite 1.
         
         // If Up(1) is not 1, and can't be 2 or 3 (taken).
         // Then Up(1) MUST be Sprite 0.
         
         // So the mapping must be:
         // Down (0) -> Sprite 3
         // Up (1) -> Sprite 0
         // Left (2) -> Sprite 2
         // Right (3) -> Sprite 1
         
         switch(face) {
             case 0: spriteFace = 3; break; // Down -> Sprite 3
             case 1: spriteFace = 0; break; // Up -> Sprite 0
             case 2: spriteFace = 2; break; // Left -> Sprite 2
             case 3: spriteFace = 1; break; // Right -> Sprite 1
         }
         
         int frame = GameManager::getInstance().getWalkFrame();
        
        // Pic Calculation: Base + Face * 7 + Frame
        int playerPic = 2501 + spriteFace * 7 + frame;
        
        spriteMap[px][py] = playerPic; 
    }

    // Render back-to-front
    static int eventLayerCount = 0; // Use static to prevent loop, but reset occasionally?
    // Actually, just log once per scene load or something.
    // Or just comment it out now that we confirmed Event 3 is found but Pic is 0.
    
    for (int i1 = 0; i1 < SCENE_MAP_SIZE; ++i1) {
        for (int i2 = 0; i2 < SCENE_MAP_SIZE; ++i2) {
            int x, y;
            GetPositionOnScreen(i1, i2, centerX, centerY, x, y);
            
            // Culling
            if (x < -200 || x > 840 || y < -200 || y > 680) continue; 

            // Layer 0: Ground
            int16_t tile0 = GetSceneTile(m_currentSceneId, 0, i1, i2);
            if (tile0 > 0) {
                // Fix: Tile index off by one (user feedback)
                // (tile / 2) - 1 maps 2 -> 0, 4 -> 1, etc.
                DrawTile(renderer, (tile0 / 2) - 1, x, y, 0, 0);
            }
            
            // Layer 1: Building
            int16_t tile1 = GetSceneTile(m_currentSceneId, 1, i1, i2);
            int16_t height1 = GetSceneTile(m_currentSceneId, 4, i1, i2);
            if (tile1 > 0) {
                DrawTile(renderer, (tile1 / 2) - 1, x, y - height1, 0, 0);
            }
            
            // Layer 2: Decor
            int16_t tile2 = GetSceneTile(m_currentSceneId, 2, i1, i2);
            int16_t height2 = GetSceneTile(m_currentSceneId, 5, i1, i2);
            if (tile2 > 0) {
                DrawTile(renderer, (tile2 / 2) - 1, x, y - height2, 0, 0);
            }
            
            // Layer 3: Event
            int16_t tile3 = GetSceneTile(m_currentSceneId, 3, i1, i2);
            if (tile3 >= 0 && tile3 < 200) { // tile3 is Event ID (0..199)
                // tile3 is eventIndex. Get EventData to find pic.
                // KYS Event Data: Index 5 is Pic
                int16_t eventPic = GetEventData(m_currentSceneId, tile3, 5);
                
                // Debug log for Scene 0 Event 3 (Kong Pili?)
                static bool loggedEvent3 = false;
                if (m_currentSceneId == 0 && tile3 == 3 && !loggedEvent3) {
                     std::cout << "[DrawScene] Scene 0 Event 3 found at Map(" << i1 << "," << i2 << ")" << std::endl;
                     std::cout << "  PicIndex (Index 5): " << eventPic << std::endl;
                     std::cout << "  Index 1: " << GetEventData(m_currentSceneId, tile3, 1) << std::endl;
                     loggedEvent3 = true;
                }
                
                // Debug log for Event 0 (Bed/Hero)
                static int lastEvent0Pic = -1;
                if (m_currentSceneId == 0 && tile3 == 0) {
                    if (eventPic != lastEvent0Pic) {
                         std::cout << "[DrawScene] Event 0 Pic changed to: " << eventPic << " (Draw: " << (eventPic/2) << ")" << std::endl;
                         lastEvent0Pic = eventPic;
                    }
                }

                if (eventPic != 0) { // Pascal logic: if <> 0 then draw
                    // Check if standing on building (Layer 1)
                    int drawY = y;
                    if (tile1 > 0) {
                        drawY -= height1;
                    }
                    
                    // DRAW EVENT
                    // Pascal Logic: 
                    // if pic > 0 then DrawSPic(pic div 2 - 1)
                    // if pic < 0 then DrawRole(-pic)
                    if (eventPic > 0) {
                        DrawSmpSprite(renderer, (eventPic / 2) - 1, x, drawY, 0);
                    } else if (eventPic < 0) {
                        DrawMmapSprite(renderer, (-eventPic / 2) - 1, x, drawY, 0);
                    }
                }
            }

            // Sprites (Player & dynamic sprites from pre-calculated map)
            if (spriteMap[i1][i2] != -1) {
                // Adjust for height too for Player
                int drawY = y;
                if (tile1 > 0) {
                    drawY -= height1;
                }
                DrawSprite(renderer, spriteMap[i1][i2], x, drawY, 0);
            }
        }
    }
    
    // Draw Clouds over the map
    DrawClouds(renderer, centerX, centerY);
}

// Global Scale for Characters
static const float CHAR_SCALE = 1.15f;

void SceneManager::DrawTile(SDL_Renderer* renderer, int picIndex, int x, int y, int offX, int offY) {
    if (picIndex < 0 || picIndex >= m_sIdxData.size()) return;
    
    int offset = m_sIdxData[picIndex];
    if (offset <= 0 || offset >= m_sPicData.size()) return; // Offset 0 is usually invalid/empty
    
    SDL_Surface* screen = GameManager::getInstance().getScreenSurface();
    if (!screen) return;
    
    GraphicsUtils::DrawRLE8(screen, x, y, &m_sPicData[offset], m_sPicData.size() - offset);
}

void SceneManager::DrawSmpSprite(SDL_Renderer* renderer, int picIndex, int x, int y, int frame) {
    if (m_sPicData.empty() || m_sIdxData.empty()) return;
    if (picIndex < 0 || picIndex >= (int)m_sIdxData.size()) return;
    
    int offset = m_sIdxData[picIndex];
    if (offset <= 0 || offset >= (int)m_sPicData.size()) return;
    
    SDL_Surface* screen = GameManager::getInstance().getScreenSurface();
    if (!screen) return;
    
    GraphicsUtils::DrawRLE8(screen, x, y, &m_sPicData[offset], m_sPicData.size() - offset, 0, CHAR_SCALE);
}

void SceneManager::DrawMmapSprite(SDL_Renderer* renderer, int picIndex, int x, int y, int frame) {
    if (m_mmapPicData.empty() || m_mmapIdxData.empty()) return;
    if (picIndex < 0 || picIndex >= (int)m_mmapIdxData.size()) return;
    
    int offset = m_mmapIdxData[picIndex];
    if (offset <= 0 || offset >= (int)m_mmapPicData.size()) return;
    
    SDL_Surface* screen = GameManager::getInstance().getScreenSurface();
    if (!screen) return;
    
    GraphicsUtils::DrawRLE8(screen, x, y, &m_mmapPicData[offset], m_mmapPicData.size() - offset, 0, CHAR_SCALE);
}

void SceneManager::DrawSprite(SDL_Renderer* renderer, int picIndex, int x, int y, int frame) {
    // This is now a dispatcher for generic calls (like player drawing)
    // For now, assume player/protagonist uses smp (matching current SceneManager::DrawScene logic)
    // but in a real KYS game, protagonist is usually in mmap.grp.
    // However, the current code (line 407) calculates playerPic as 2501 + ..., 
    // and 2501 is typically an smp index.
    DrawSmpSprite(renderer, picIndex, x, y, frame);
}



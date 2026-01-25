#pragma once
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include "Scene.h"
#include "GameTypes.h" // Assuming this exists or I should create it for common types

// Constants
constexpr int MAX_SCENES = 100; // Adjust as needed
constexpr int SCENE_MAP_SIZE = 64;
constexpr int SCENE_LAYERS = 6;

class SceneManager {
public:
    static SceneManager& getInstance();

    bool Init();
    bool LoadEventData(const std::string& path); // Loads Event Data (DData) from alldef.grp
    bool LoadMapData(const std::string& path); // Loads Map data (SData) from allsin.grp
    
    // Save Data
    bool SaveEventData(const std::string& path);
    bool SaveMapData(const std::string& path);

    // Set Scenes (loaded from ranger.grp/save file)
    void SetScenes(const std::vector<Scene>& scenes);
    
    // Core drawing function
    // centerX, centerY: Tile coordinates of the camera center (player position)
    void DrawScene(SDL_Renderer* renderer, int centerX, int centerY);
    
    // Helpers
    void SetCurrentScene(int sceneId);
    int GetCurrentSceneId() const { return m_currentSceneId; }
    
    // Movement & Collision
    bool CanWalk(int x, int y);
    void GetPositionOnScreen(int mapX, int mapY, int centerX, int centerY, int& outX, int& outY);
    
    // Animation & Effects
    void Update(uint32_t ticks); // Update animations (clouds, water, etc.)
    void DrawClouds(SDL_Renderer* renderer, int centerX, int centerY);
    
    // Accessors for SData equivalent
    int16_t GetSceneTile(int sceneId, int layer, int x, int y) const;
    void SetSceneTile(int sceneId, int layer, int x, int y, int16_t value);
    
    // Update Event Position in Map (Layer 3)
    void UpdateEventPosition(int sceneId, int eventId, int oldX, int oldY, int newX, int newY);

    // Accessors for DData (Event Definitions)
    // Pascal: DData[SceneId, EventId, Index]
    // EventId: 0..199, Index: 0..10
    int16_t GetEventData(int sceneId, int eventId, int index) const;
    void SetEventData(int sceneId, int eventId, int index, int16_t value);

    // Accessors for Scene definitions
    Scene* GetScene(int sceneId);
    
    // Resource Loading
    bool LoadResources();
    
    // Testing Helper
    void CreateMockScene(int id);

    // Helper to draw a single tile (from smp)
    void DrawTile(SDL_Renderer* renderer, int picIndex, int x, int y, int offX, int offY);
    
    // Helper to draw a sprite from smp (static objects)
    void DrawSmpSprite(SDL_Renderer* renderer, int picIndex, int x, int y, int frame = 0);

    // Helper to draw a sprite from mmap.grp (roles/NPCs)
    void DrawMmapSprite(SDL_Renderer* renderer, int picIndex, int x, int y, int frame = 0);

    // Legacy/Generic sprite draw (decides between smp/mmap)
    void DrawSprite(SDL_Renderer* renderer, int picIndex, int x, int y, int frame = 0);

private:
    SceneManager();
    ~SceneManager() = default;
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    // World Map Data (Layout)
    // Usually 480x480, 2 layers?
    // Let's assume KYS standard: 480x480 * 2 bytes/tile * 2 layers = 921600 bytes.
    // Or just 1 layer?
    // EARTH.002 is typically the map file.
    std::vector<int16_t> m_worldMapData;
    int m_worldMapWidth = 480;
    int m_worldMapHeight = 480;
    
    // World Map Resources
    std::vector<uint8_t> m_wPicData; // wmp
    std::vector<int32_t> m_wIdxData; // wdx

    // Helper to draw World Map
    void DrawWorldMap(SDL_Renderer* renderer, int centerX, int centerY);
    bool LoadWorldMap();

    // Scene Definitions
    std::vector<Scene> m_scenes;
    
    // Map Data: [SceneId][Layer][Y][X]
    std::vector<int16_t> m_mapData;

    // Event Data: [SceneId][EventId][Index]
    // EventId max 200, Index max 11
    struct EventData {
        int16_t data[200][11];
    };
    std::vector<EventData> m_eventData;
    
    // Tile Graphics (RLE) - smp/sdx
    std::vector<uint8_t> m_sPicData; // smp
    std::vector<int32_t> m_sIdxData; // sdx
    
    // Sprite Graphics (RLE) - mmap.grp/mmap.idx
    std::vector<uint8_t> m_mmapPicData; 
    std::vector<int32_t> m_mmapIdxData;

    // Cloud Data
    struct Cloud {
        int x, y;
        int speedX, speedY;
        int picNum;
        int shadow;
        int alpha;
    };
    std::vector<Cloud> m_clouds;
    std::vector<uint8_t> m_cloudPicData; // cloud.grp
    std::vector<int32_t> m_cloudIdxData; // cloud.idx
    
    int m_currentSceneId;
    
    // Timer state
    uint32_t m_lastWaterUpdate;
    uint32_t m_lastCloudUpdate;
};

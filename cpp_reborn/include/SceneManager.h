#pragma once
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include "Scene.h"
#include "GameTypes.h" 

// 常量定义
constexpr int MAX_SCENES = 100; // 最大场景数
constexpr int SCENE_MAP_SIZE = 64; // 场景地图尺寸 (64x64)
constexpr int SCENE_LAYERS = 6; // 场景层数 (地面, 建筑, 物品, 事件, 遮挡, 飞行)

// 场景管理器 - 对应 Pascal 原版 kys_main.pas 中的场景管理逻辑 (SData, DData) 及渲染
class SceneManager {
public:
    static SceneManager& getInstance();

    bool Init();
    
    // 加载事件数据 (DData) - 对应文件 alldef.grp
    // Pascal: Read(f, DData)
    bool LoadEventData(const std::string& path); 
    
    // 加载地图数据 (SData) - 对应文件 allsin.grp
    // Pascal: Read(f, SData)
    bool LoadMapData(const std::string& path); 
    
    // 保存数据
    bool SaveEventData(const std::string& path);
    bool SaveMapData(const std::string& path);

    // 设置场景列表 (从 ranger.grp 或存档加载)
    void SetScenes(const std::vector<Scene>& scenes);
    
    // 核心绘制函数
    // centerX, centerY: 摄像机中心对应的Tile坐标 (通常是主角位置)
    // 对应 Pascal 中的 DrawMap/DrawScene 逻辑
    void DrawScene(SDL_Renderer* renderer, int centerX, int centerY);
    
    // 辅助函数
    void SetCurrentScene(int sceneId);
    int GetCurrentSceneId() const { return m_currentSceneId; }
    
    // 移动与碰撞检测
    // 对应 Pascal 中的 CanWalk / Walk 逻辑
    bool CanWalk(int x, int y);
    
    // 坐标转换: 地图坐标 -> 屏幕坐标
    void GetPositionOnScreen(int mapX, int mapY, int centerX, int centerY, int& outX, int& outY);
    
    // 动画与特效更新 (云层, 水面等)
    void Update(uint32_t ticks); 
    void DrawClouds(SDL_Renderer* renderer, int centerX, int centerY);
    
    // 访问 SData (场景地图数据)
    // Pascal: SData[SceneId, Layer, X, Y]
    int16_t GetSceneTile(int sceneId, int layer, int x, int y) const;
    void SetSceneTile(int sceneId, int layer, int x, int y, int16_t value);
    
    // 更新地图上的事件位置 (Layer 3)
    void UpdateEventPosition(int sceneId, int eventId, int oldX, int oldY, int newX, int newY);

    // 访问 DData (场景事件定义)
    // Pascal: DData[SceneId, EventId, Index]
    // EventId: 0..199, Index: 0..10
    int16_t GetEventData(int sceneId, int eventId, int index) const;
    void SetEventData(int sceneId, int eventId, int index, int16_t value);

    // 获取场景定义对象
    Scene* GetScene(int sceneId);
    
    // 加载资源 (贴图等)
    bool LoadResources();
    
    // 测试辅助
    void CreateMockScene(int id);

    // 绘制单个图块 (来自 smp)
    void DrawTile(SDL_Renderer* renderer, int picIndex, int x, int y, int offX, int offY);
    
    // 绘制精灵 (来自 smp, 静态物体)
    void DrawSmpSprite(SDL_Renderer* renderer, int picIndex, int x, int y, int frame = 0);

    // 绘制精灵 (来自 mmap.grp, 角色/NPC)
    void DrawMmapSprite(SDL_Renderer* renderer, int picIndex, int x, int y, int frame = 0);

    // 绘制精灵 (来自 Scene.Pic, 动态物体)
    void DrawScenePicSprite(SDL_Renderer* renderer, int picIndex, int x, int y, int frame = 0);

    // 通用精灵绘制 (根据 picIndex 自动判断来源)
    void DrawSprite(SDL_Renderer* renderer, int picIndex, int x, int y, int frame = 0);

private:
    SceneManager();
    ~SceneManager() = default;
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    // 大地图数据 (World Map)
    // 对应 EARTH.002
    std::vector<int16_t> m_worldMapData;
    int m_worldMapWidth = 480;
    int m_worldMapHeight = 480;
    
    // 战斗地图资源 (WarMap) - wmp/wdx
    std::vector<uint8_t> m_wmpPicData; // wmp
    std::vector<int32_t> m_wmpIdxData; // wdx

    // 绘制大地图
    void DrawWorldMap(SDL_Renderer* renderer, int centerX, int centerY);
    bool LoadWorldMap();

    // 场景定义列表
    std::vector<Scene> m_scenes;
    
    // 地图数据 SData: [SceneId][Layer][Y][X]
    // 扁平化存储以优化性能
    std::vector<int16_t> m_mapData;

    // 事件数据 DData: [SceneId][EventId][Index]
    // EventId max 200, Index max 11
    struct EventData {
        int16_t data[200][11];
    };
    std::vector<EventData> m_eventData;
    
    // 场景图块资源 (SceneMap) - smp/sdx
    std::vector<uint8_t> m_smpPicData; // smp
    std::vector<int32_t> m_smpIdxData; // sdx
    
    // 大地图资源 (MaxMap) - mmap.grp/mmap.idx
    std::vector<uint8_t> m_mmpPicData; // mmp
    std::vector<int32_t> m_mmpIdxData; // midx

    // 动态场景贴图 (Scene.Pic) - PNG 格式集合
    struct ScenePic {
        int x, y, black;
        SDL_Surface* surface = nullptr;
    };
    std::vector<ScenePic> m_scenePics;

    // 云层数据
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
    
    // 计时器状态
    uint32_t m_lastWaterUpdate;
    uint32_t m_lastCloudUpdate;
};

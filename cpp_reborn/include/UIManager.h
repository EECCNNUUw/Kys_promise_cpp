#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <vector>
#include <cstdint>

class Role;

// UI 管理器 - 对应 Pascal 原版 kys_engine.pas 及部分 kys_main.pas 的 UI 逻辑
// 负责处理游戏界面绘制、菜单显示、对话框、文字渲染等
class UIManager {
public:
    static UIManager& getInstance();

    bool Init(SDL_Renderer* renderer, SDL_Window* window);
    void Cleanup();

    // Resource Loading
    // 加载系统图形资源 (Background.Pic)
    bool LoadSystemGraphics(); 
    // 播放标题动画 (Begin.Pic)
    void PlayTitleAnimation(); 
    // 绘制标题画面 (Background.Pic index 0)
    void DrawTitleScreen();    
    // 绘制标题背景 (Begin.Pic 的最后一帧)
    void DrawTitleBackground(); 

    // Core UI Drawing
    // 绘制矩形框 (对应 Pascal DrawFrame)
    void DrawRectangle(int x, int y, int w, int h, uint32_t colorin, uint32_t colorframe, int alpha);
    // 绘制居中纹理
    void DrawCenteredTexture(SDL_Texture* tex);
    // 绘制文字 (假设输入为 GBK 编码)
    void DrawText(const std::string& text, int x, int y, uint32_t color, int fontSize = 20); 
    // 绘制文字 (假设输入为 UTF-8 编码)
    void DrawTextUtf8(const std::string& text, int x, int y, uint32_t color, int fontSize = 20); 
    // 绘制带阴影文字 (GBK)
    void DrawShadowText(const std::string& text, int x, int y, uint32_t color1, uint32_t color2, int fontSize = 20); 
    // 绘制带阴影文字 (UTF-8)
    void DrawShadowTextUtf8(const std::string& text, int x, int y, uint32_t color1, uint32_t color2, int fontSize = 20); 
    
    // Draw Head Portrait
    // 绘制头像 (Head.img/Head.grp)
    void DrawHead(int headId, int x, int y);

    // Character Creation UI
    // 显示创建角色界面
    void ShowCharacterCreation(const Role& role);

    // Menu System
    // 显示主菜单 (保存/读取/离开) - 阻塞式
    void ShowMenu(); 
    // 渲染一帧菜单系统
    void RenderMenuSystem(int menuSelection); 
    
    // 状态显示相关
    void SelectShowStatus();
    void ShowStatus(int roleId); // 显示角色状态面板
    
    // 武功显示相关
    void SelectShowMagic();
    void ShowMagic(int roleId, int selectedIndex = -1);
    
    // 系统菜单显示相关
    void SelectShowSystem();
    void ShowSystem(int selectedIndex);
    
    // 技能/修炼显示相关
    void SelectShowSkill();
    void ShowSkill(int petId, int selectedIndex);
    
    // 队友选择相关
    void SelectShowTeammate();
    void ShowTeammate(int tMenu, int rMenu, int position);
    
    // 物品显示相关
    void SelectShowItem();
    void ShowItem(int menuSelection);
    
    // 存档/读档菜单
    void ShowSaveLoadMenu(bool isSave); 
    
    // 显示对话框
    // text: 对话内容
    // headId: 头像ID
    // mode: 显示模式
    void ShowDialogue(const std::string& text, int headId, int mode, const std::string& nameUtf8 = "", const std::string& nameRawBytes = "");
    
    // 显示标题文字
    void ShowTitle(const std::string& text, int x, int y, uint32_t color1, uint32_t color2);
    
    // 显示选择框 (是/否)
    // 返回: 1=Yes, 0=No/Cancel
    int ShowChoice(const std::string& text); 
    
    // 显示物品获得提示
    void ShowItemNotification(int itemId, int amount);
    
    // Screen Effects
    // 屏幕淡入淡出 (fadeIn=true 淡入, false 淡出)
    void FadeScreen(bool fadeIn);
    // 屏幕闪烁
    void FlashScreen(uint32_t color, int durationMs);

    // Capture current screen content to m_texMenuBackground
    void CaptureScreen();

    // Force screen update (for blocking loops)
    // 强制刷新屏幕 (用于阻塞循环中)
    void UpdateScreen();

    SDL_Renderer* GetRenderer() const { return m_renderer; }

private:
    UIManager();
    ~UIManager() = default;
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    SDL_Renderer* m_renderer = nullptr;
    SDL_Window* m_window = nullptr;
    TTF_Font* m_font = nullptr;

    // Helper to get color from uint32 (AARRGGBB or similar)
    // 辅助函数: uint32 转 SDL_Color
    SDL_Color Uint32ToColor(uint32_t color);

    // System Graphics from Background.Pic
    // 系统图形资源
    SDL_Texture* m_texTitle = nullptr;    // Index 0: 标题画面
    SDL_Texture* m_texMagic = nullptr;    // Index 1: 武功界面背景
    SDL_Texture* m_texState = nullptr;    // Index 2: 状态界面背景
    SDL_Texture* m_texSystem = nullptr;   // Index 3: 系统菜单背景
    SDL_Texture* m_texMap = nullptr;      // Index 4: 地图背景
    SDL_Texture* m_texSkill = nullptr;    // Index 5: 技能界面背景
    SDL_Texture* m_texMenuEsc = nullptr;  // Index 6: 菜单图标
    SDL_Texture* m_texMenuEscBack = nullptr; // Index 7: 菜单背景
    SDL_Texture* m_texBattle = nullptr;   // Index 8: 战斗界面背景
    SDL_Texture* m_texTeammate = nullptr; // Index 9: 队友选择背景
    SDL_Texture* m_texMenuItem = nullptr; // Index 10: 物品界面背景
    
    SDL_Texture* m_texBeginBackground = nullptr; // Begin.Pic 最后一帧 (标题背景)
    SDL_Texture* m_texMenuBackground = nullptr; // 菜单半透明背景 (截图)

    // Helper to load texture from surface and free surface
    SDL_Texture* LoadTextureFromSurface(SDL_Surface* surface);
};

#endif // UIMANAGER_H

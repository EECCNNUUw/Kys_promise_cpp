#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <vector>
#include <cstdint>

class Role;

class UIManager {
public:
    static UIManager& getInstance();

    bool Init(SDL_Renderer* renderer, SDL_Window* window);
    void Cleanup();

    // Resource Loading
    bool LoadSystemGraphics(); // Loads Background.Pic and others
    void PlayTitleAnimation(); // Plays Begin.Pic
    void DrawTitleScreen();    // Draws Background.Pic index 0
    void DrawTitleBackground(); // Draws the last frame of Begin.Pic

    // Core UI Drawing
    void DrawRectangle(int x, int y, int w, int h, uint32_t colorin, uint32_t colorframe, int alpha);
    void DrawCenteredTexture(SDL_Texture* tex);
    void DrawText(const std::string& text, int x, int y, uint32_t color, int fontSize = 20); // Assumes GBK input
    void DrawTextUtf8(const std::string& text, int x, int y, uint32_t color, int fontSize = 20); // Assumes UTF-8 input
    void DrawShadowText(const std::string& text, int x, int y, uint32_t color1, uint32_t color2, int fontSize = 20); // Assumes GBK
    void DrawShadowTextUtf8(const std::string& text, int x, int y, uint32_t color1, uint32_t color2, int fontSize = 20); // Assumes UTF-8
    
    // Draw Head Portrait
    void DrawHead(int headId, int x, int y);

    // Character Creation UI
    void ShowCharacterCreation(const Role& role);

    // Menu System
    void ShowMenu(); // Main game menu (Save, Load, etc.) - Blocking
    void RenderMenuSystem(int menuSelection); // Render one frame of menu
    void SelectShowStatus();
    void ShowStatus(int roleId);
    void SelectShowMagic();
    void ShowMagic(int roleId, int selectedIndex = -1);
    void SelectShowSystem();
    void ShowSystem(int selectedIndex);
    void SelectShowSkill();
    void ShowSkill(int petId, int selectedIndex);
    void SelectShowTeammate();
    void ShowTeammate(int tMenu, int rMenu, int position);
    void SelectShowItem();
    void ShowItem(int menuSelection);
    void ShowSaveLoadMenu(bool isSave); // Shows save/load slots
    void ShowDialogue(const std::string& text, int headId, int mode, const std::string& nameUtf8 = "", const std::string& nameRawBytes = "");
    void ShowTitle(const std::string& text, int x, int y, uint32_t color1, uint32_t color2);
    int ShowChoice(const std::string& text); // Returns 0 for No/Cancel, 1 for Yes/Confirm
    void ShowItemNotification(int itemId, int amount);
    
    // Screen Effects
    void FadeScreen(bool fadeIn);
    void FlashScreen(uint32_t color, int durationMs);

    // Force screen update (for blocking loops)
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
    // Pascal code seems to use 32-bit int for color. 
    // Need to verify color format. SDL3 usually uses SDL_Color.
    SDL_Color Uint32ToColor(uint32_t color);

    // System Graphics from Background.Pic
    SDL_Texture* m_texTitle = nullptr;    // Index 0
    SDL_Texture* m_texMagic = nullptr;    // Index 1
    SDL_Texture* m_texState = nullptr;    // Index 2
    SDL_Texture* m_texSystem = nullptr;   // Index 3
    SDL_Texture* m_texMap = nullptr;      // Index 4
    SDL_Texture* m_texSkill = nullptr;    // Index 5
    SDL_Texture* m_texMenuEsc = nullptr;  // Index 6
    SDL_Texture* m_texMenuEscBack = nullptr; // Index 7
    SDL_Texture* m_texBattle = nullptr;   // Index 8
    SDL_Texture* m_texTeammate = nullptr; // Index 9
    SDL_Texture* m_texMenuItem = nullptr; // Index 10
    
    SDL_Texture* m_texBeginBackground = nullptr; // Last frame of Begin.Pic
    SDL_Texture* m_texMenuBackground = nullptr; // Captured screen for transparency

    // Helper to load texture from surface and free surface
    SDL_Texture* LoadTextureFromPic(const std::string& filename, int index);
    SDL_Texture* SurfaceToTexture(SDL_Surface* surface);

    // Capture Screen Helper
    void CaptureScreen();
    void ShowDialogueWithCapture(const std::string& text, int headId, int mode);

private:
};

#endif // UIMANAGER_H

#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <vector>

// Define this if you have SDL_ttf available
#define USE_SDL_TTF

#ifdef USE_SDL_TTF
#include <SDL3_ttf/SDL_ttf.h>
#endif

class TextManager {
public:
    static TextManager& getInstance();

    bool Init();
    void Quit();

    // Load a font (size is in points)
    // Returns true if successful
    bool loadFont(const std::string& fontPath, int size);

    // Render text to the internal surface of GameManager (or a specific surface)
    // text: GBK encoded string (from game data)
    // x, y: Position
    // color: 32-bit color (0xAARRGGBB)
    void RenderText(const std::string& text, int x, int y, uint32_t color);
    
    // Render UTF-8 text directly
    void RenderTextUtf8(const std::string& text, int x, int y, SDL_Color color, int fontSize = 20);

    // Convert GBK string to UTF-8 string
    std::string gbkToUtf8(const std::string& gbkStr);

private:
    TextManager();
    ~TextManager();

#ifdef USE_SDL_TTF
    TTF_Font* m_font;
#else
    void* m_font; // Placeholder
#endif
};

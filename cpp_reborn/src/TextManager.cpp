#include "TextManager.h"
#include "GameManager.h"
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

TextManager& TextManager::getInstance() {
    static TextManager instance;
    return instance;
}

TextManager::TextManager() : m_font(nullptr) {
}

TextManager::~TextManager() {
    Quit();
}

bool TextManager::Init() {
#ifdef USE_SDL_TTF
    if (!TTF_Init()) {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }
#else
    std::cout << "TextManager: SDL_ttf not enabled. Text rendering will be simulated." << std::endl;
#endif
    return true;
}

void TextManager::Quit() {
#ifdef USE_SDL_TTF
    if (m_font) {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
    TTF_Quit();
#endif
}

bool TextManager::loadFont(const std::string& fontPath, int size) {
#ifdef USE_SDL_TTF
    if (m_font) {
        TTF_CloseFont(m_font);
    }
    m_font = TTF_OpenFont(fontPath.c_str(), static_cast<float>(size));
    if (!m_font) {
        std::cerr << "Failed to load font: " << fontPath << " Error: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
#else
    std::cout << "TextManager: Loading font (simulated): " << fontPath << std::endl;
    return true;
#endif
}

std::string TextManager::gbkToUtf8(const std::string& gbkStr) {
    if (gbkStr.empty()) return "";

#ifdef _WIN32
    // 1. GBK (CP936) -> UTF-16
    int targetLen = MultiByteToWideChar(936, 0, gbkStr.c_str(), -1, NULL, 0);
    if (targetLen == 0) return gbkStr; // Conversion failed

    std::vector<wchar_t> wBuf(targetLen);
    MultiByteToWideChar(936, 0, gbkStr.c_str(), -1, wBuf.data(), targetLen);

    // 2. UTF-16 -> UTF-8
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, NULL, 0, NULL, NULL);
    if (utf8Len == 0) return gbkStr;

    std::vector<char> utf8Buf(utf8Len);
    WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, utf8Buf.data(), utf8Len, NULL, NULL);

    return std::string(utf8Buf.data());
#else
    // Non-Windows fallback (e.g., iconv could be used here)
    return gbkStr; 
#endif
}

void TextManager::RenderText(const std::string& text, int x, int y, uint32_t color) {
    std::string utf8Text = gbkToUtf8(text);

#ifdef USE_SDL_TTF
    if (!m_font) return;

    SDL_Color sdlColor;
    sdlColor.r = (color >> 16) & 0xFF;
    sdlColor.g = (color >> 8) & 0xFF;
    sdlColor.b = color & 0xFF;
    sdlColor.a = (color >> 24) & 0xFF;

    SDL_Surface* textSurface = TTF_RenderText_Solid(m_font, utf8Text.c_str(), 0, sdlColor);
    if (textSurface) {
        SDL_Surface* screen = GameManager::getInstance().getScreenSurface();
        if (screen) {
            SDL_Rect destRect = { x, y, textSurface->w, textSurface->h };
            SDL_BlitSurface(textSurface, NULL, screen, &destRect);
        }
        SDL_DestroySurface(textSurface);
    }
#else
    // Simulation: Print to console
    // std::cout << "DrawText at (" << x << "," << y << "): " << utf8Text << std::endl;
#endif
}

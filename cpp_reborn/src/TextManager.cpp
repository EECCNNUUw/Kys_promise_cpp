#include "TextManager.h"
#include "GameManager.h"
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
    std::string TrimTrailingJunkBytes(std::string s) {
        while (!s.empty()) {
            unsigned char b = static_cast<unsigned char>(s.back());
            if (b == 0 || b == 0x2A || b <= 0x20 || b == 0x7F || b == 0xA0 || b == 0xFF || b == 0xFE) {
                s.pop_back();
                continue;
            }
            break;
        }
        size_t zeroPos = s.find('\0');
        if (zeroPos != std::string::npos) s.resize(zeroPos);
        return s;
    }

    void StripTrailingUtf8ReplacementChar(std::string& s) {
        static constexpr unsigned char kReplacement[] = { 0xEF, 0xBF, 0xBD };
        while (s.size() >= 3) {
            size_t n = s.size();
            if (static_cast<unsigned char>(s[n - 3]) == kReplacement[0] &&
                static_cast<unsigned char>(s[n - 2]) == kReplacement[1] &&
                static_cast<unsigned char>(s[n - 1]) == kReplacement[2]) {
                s.resize(n - 3);
                continue;
            }
            break;
        }
        s = TrimTrailingJunkBytes(std::move(s));
    }

#ifdef _WIN32
    bool IsValidUtf8(const std::string& s) {
        if (s.empty()) return true;
        int wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), NULL, 0);
        return wideLen != 0;
    }

    bool ConvertMultiByteToUtf8(UINT codePage, const std::string& bytes, std::string& outUtf8) {
        outUtf8.clear();
        if (bytes.empty()) return true;
        int wideLen = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), NULL, 0);
        if (wideLen == 0) return false;
        std::vector<wchar_t> wBuf(static_cast<size_t>(wideLen));
        if (MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), wBuf.data(), wideLen) == 0) return false;

        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), wideLen, NULL, 0, NULL, NULL);
        if (utf8Len == 0) return false;
        std::string utf8(static_cast<size_t>(utf8Len), '\0');
        if (WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), wideLen, utf8.data(), utf8Len, NULL, NULL) == 0) return false;
        outUtf8 = std::move(utf8);
        return true;
    }
#else
    bool IsValidUtf8(const std::string&) { return false; }
#endif
}

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
    auto convert = [&](UINT codePage) -> std::string {
        int targetLen = MultiByteToWideChar(codePage, 0, gbkStr.c_str(), -1, NULL, 0);
        if (targetLen == 0) return "";

        std::vector<wchar_t> wBuf(targetLen);
        if (MultiByteToWideChar(codePage, 0, gbkStr.c_str(), -1, wBuf.data(), targetLen) == 0) return "";

        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, NULL, 0, NULL, NULL);
        if (utf8Len == 0) return "";

        std::vector<char> utf8Buf(utf8Len);
        if (WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, utf8Buf.data(), utf8Len, NULL, NULL) == 0) return "";

        return std::string(utf8Buf.data());
    };

    std::string utf8 = convert(936);
    if (!utf8.empty()) return utf8;
    utf8 = convert(950);
    if (!utf8.empty()) return utf8;
    return gbkStr;
#else
    // Non-Windows fallback (e.g., iconv could be used here)
    return gbkStr; 
#endif
}

std::string TextManager::big5ToUtf8(const std::string& big5Str) {
    if (big5Str.empty()) return "";

#ifdef _WIN32
    auto convert = [&](UINT codePage) -> std::string {
        int targetLen = MultiByteToWideChar(codePage, 0, big5Str.c_str(), -1, NULL, 0);
        if (targetLen == 0) return "";

        std::vector<wchar_t> wBuf(targetLen);
        if (MultiByteToWideChar(codePage, 0, big5Str.c_str(), -1, wBuf.data(), targetLen) == 0) return "";

        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, NULL, 0, NULL, NULL);
        if (utf8Len == 0) return "";

        std::vector<char> utf8Buf(utf8Len);
        if (WideCharToMultiByte(CP_UTF8, 0, wBuf.data(), -1, utf8Buf.data(), utf8Len, NULL, NULL) == 0) return "";

        return std::string(utf8Buf.data());
    };

    std::string utf8 = convert(950);
    if (!utf8.empty()) return utf8;
    utf8 = convert(936);
    if (!utf8.empty()) return utf8;
    return big5Str;
#else
    return big5Str;
#endif
}

std::string TextManager::nameToUtf8(const std::string& ansiStr) {
    if (ansiStr.empty()) return "";

    std::string cleaned = TrimTrailingJunkBytes(ansiStr);
    if (cleaned.empty()) return "";

    std::string utf8;
    if (IsValidUtf8(cleaned)) {
        utf8 = std::move(cleaned);
    } else {
        bool ok = false;
#ifdef _WIN32
        std::string attempt = cleaned;
        while (!attempt.empty()) {
            if (ConvertMultiByteToUtf8(936, attempt, utf8)) {
                ok = true;
                break;
            }
            attempt.pop_back();
        }
        if (!ok) {
            attempt = cleaned;
            while (!attempt.empty()) {
                if (ConvertMultiByteToUtf8(950, attempt, utf8)) {
                    ok = true;
                    break;
                }
                attempt.pop_back();
            }
        }
#endif
        if (!ok) {
            utf8.clear();
        }
    }

    StripTrailingUtf8ReplacementChar(utf8);
    return utf8;
}

std::string TextManager::utf8ToGbk(const std::string& utf8Str) {
    if (utf8Str.empty()) return "";

#ifdef _WIN32
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    if (wideLen == 0) return utf8Str;

    std::vector<wchar_t> wBuf(wideLen);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, wBuf.data(), wideLen);

    auto convert = [&](UINT codePage) -> std::string {
        int len = WideCharToMultiByte(codePage, 0, wBuf.data(), -1, NULL, 0, NULL, NULL);
        if (len == 0) return "";
        std::vector<char> buf(len);
        if (WideCharToMultiByte(codePage, 0, wBuf.data(), -1, buf.data(), len, NULL, NULL) == 0) return "";
        return std::string(buf.data());
    };

    std::string ansi = convert(936);
    if (!ansi.empty()) return ansi;
    ansi = convert(950);
    if (!ansi.empty()) return ansi;
    return utf8Str;
#else
    return utf8Str;
#endif
}

void TextManager::RenderText(const std::string& text, int x, int y, uint32_t color) {
    std::string utf8Text = gbkToUtf8(text);
    RenderTextUtf8(utf8Text, x, y, color);
}

void TextManager::RenderTextUtf8(const std::string& text, int x, int y, uint32_t color, int fontSize) {
    if (text.empty()) return;

    SDL_Color sdlColor;
    sdlColor.r = (color >> 16) & 0xFF;
    sdlColor.g = (color >> 8) & 0xFF;
    sdlColor.b = color & 0xFF;
    sdlColor.a = (color >> 24) & 0xFF;

#ifdef USE_SDL_TTF
    if (!m_font) return;

    // Note: fontSize is currently ignored because we use the pre-loaded m_font size.
    // To support multiple font sizes, we would need a font cache.
    
    SDL_Surface* textSurface = TTF_RenderText_Blended(m_font, text.c_str(), 0, sdlColor);
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
    // std::cout << "DrawTextUtf8 at (" << x << "," << y << "): " << text << std::endl;
#endif
}

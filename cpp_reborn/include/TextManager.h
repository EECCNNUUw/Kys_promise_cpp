#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <vector>

// 定义是否使用 SDL_ttf
#define USE_SDL_TTF

#ifdef USE_SDL_TTF
#include <SDL3_ttf/SDL_ttf.h>
#endif

// 文本管理器 - 负责字体加载与文本渲染
// 对应 Pascal 原版中的文字显示逻辑 (处理 GBK/Big5 编码)
class TextManager {
public:
    static TextManager& getInstance();

    bool Init();
    void Quit();

    // 加载字体 (size 单位为点)
    // 对应 Pascal: 加载 HZK16/ASC16 或系统字体
    bool loadFont(const std::string& fontPath, int size);

    // 渲染文本到 GameManager 的内部表面或屏幕
    // text: GBK 编码字符串 (来自原始游戏数据)
    // x, y: 坐标
    // color: 32位颜色 (0xAARRGGBB)
    void RenderText(const std::string& text, int x, int y, uint32_t color);
    
    // 直接渲染 UTF-8 文本
    void RenderTextUtf8(const std::string& text, int x, int y, uint32_t color, int fontSize = 20);

    // 编码转换辅助函数
    // 原始数据通常为 GBK 或 Big5，SDL3/Modern C++ 需要 UTF-8
    std::string gbkToUtf8(const std::string& gbkStr);
    std::string big5ToUtf8(const std::string& big5Str);
    
    // 智能转换名称 (自动检测编码)
    std::string nameToUtf8(const std::string& ansiStr);

    std::string utf8ToGbk(const std::string& utf8Str);

private:
    TextManager();
    ~TextManager();

#ifdef USE_SDL_TTF
    TTF_Font* m_font;
#else
    void* m_font; // 占位符
#endif
};

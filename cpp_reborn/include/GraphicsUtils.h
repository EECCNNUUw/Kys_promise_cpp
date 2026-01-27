#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include "GameTypes.h"

// 图形工具类 - 对应 Pascal 原版 kys_gfx.pas 及 M_Gfx.pas
// 处理调色板管理、像素绘制、RLE解码等底层图形操作
struct Color {
    uint8_t r, g, b;
};

class GraphicsUtils {
public:
    // Palette Management
    // 加载调色板文件 (.pal)
    static void loadPalette(const std::string& filename);
    // 重置调色板 (通常重置为默认色)
    static void resetPalette(int index = 0);
    // 获取指定索引的 32位 颜色值 (ARGB)
    static uint32_t getPaletteColor(int index); 
    
    // Palette Animation
    // 调色板轮换动画 (用于水面流动效果等)
    static void ChangeCol(uint32_t ticks); 
    
    // Drawing Primitives
    // 绘制单个像素
    static void DrawPixel(SDL_Surface* surface, int x, int y, uint32_t color);
    // 获取单个像素颜色
    static uint32_t GetPixel(SDL_Surface* surface, int x, int y);
    
    // RLE Rendering
    // RLE 解码并绘制 (Run-Length Encoding)
    // 对应 Pascal 中的 DrawMap/PutMap 等函数
    // dest: 目标表面
    // x, y: 绘制坐标
    // rawData: 原始 RLE 压缩数据 (从 .idx 索引处开始)
    // dataSize: 数据大小
    // shadow: 是否绘制阴影 (0=正常, 1=半透明阴影?)
    // scale: 缩放比例
    static void DrawRLE8(SDL_Surface* dest, int x, int y, const uint8_t* rawData, size_t dataSize, int shadow = 0, float scale = 1.0f);
    
private:
    static std::vector<uint8_t> m_fullPaletteData; // 完整调色板数据 (4 * 256 * 3 bytes, 可能是多组调色板?)
    static std::vector<uint32_t> m_currentPaletteRGBA; // 当前调色板的 32位 RGBA 缓存
    
    // Internal helper to map RGB to 32-bit format of the surface
    // 辅助函数: 将 RGB 映射为 32位 ARGB 格式
    static uint32_t mapRGB(uint8_t r, uint8_t g, uint8_t b);
};

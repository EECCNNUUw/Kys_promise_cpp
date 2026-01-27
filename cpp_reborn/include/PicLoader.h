#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <memory>

struct PicImage {
    SDL_Surface* surface = nullptr;
    int x = 0;
    int y = 0;
    int black = 0;

    // Helper to free surface automatically if we wanted, 
    // but for now we keep it raw to match other parts of the engine
    // or we should manage it. 
    // In Pascal TPic has a 'pic' field which is PSDL_Surface.
};

// 图像资源加载器 - 对应 Pascal 原版处理 .pic/.grp 文件的逻辑
// 负责读取打包的图像资源文件，并提取单帧图像
class PicLoader {
public:
    // Load a .pic file and retrieve the image at index 'num'
    // 加载 .pic/.grp 文件并获取指定索引的图像
    // filename: 文件路径 (如 resource/Head.img)
    // num: 图片编号
    static PicImage loadPic(const std::string& filename, int num);

    // Get the number of images in the .pic file
    // 获取 .pic/.grp 文件中包含的图片总数
    static int getPicCount(const std::string& filename);

    // Free the surface inside PicImage
    // 释放图片资源
    static void freePic(PicImage& pic);
};

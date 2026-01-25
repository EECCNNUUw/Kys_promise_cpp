#include "PicLoader.h"
#include <SDL3_image/SDL_image.h>
#include <fstream>
#include <iostream>
#include "FileLoader.h"

int PicLoader::getPicCount(const std::string& filename) {
    std::string path = FileLoader::getResourcePath(filename);
    std::ifstream file(path, std::ios::binary);
    
    if (!file.is_open()) {
        return 0;
    }

    int32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), 4);
    return count;
}

PicImage PicLoader::loadPic(const std::string& filename, int num) {
    PicImage result;
    
    // Ensure filename has resource path if needed
    // Assuming FileLoader::getResourcePath handles prefixing if not present
    // But usually we pass "resource/heads.pic" directly or "heads.pic"
    // Let's assume the caller passes the correct relative path, and getResourcePath ensures it's found relative to executable or root.
    std::string path = FileLoader::getResourcePath(filename);
    std::ifstream file(path, std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "PicLoader: Failed to open file: " << path << std::endl;
        return result;
    }

    // Read Count (Number of images)
    int32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), 4);
    
    // Check if num is within valid range (assuming 0-indexed)
    // Note: In Pascal, sometimes indices are 1-based, but here the logic implies:
    // offsets table size is (Count+1)*4.
    // offsets[0] is Count.
    // offsets[1] is end of image 0.
    // offsets[num+1] is end of image num.
    // So if num = count, we read offset[count+1] which is out of bounds if table is size (Count+1).
    // So valid num is 0 to Count - 1.
    
    // However, if count is just a header value and the table follows...
    // Let's assume num is 0-indexed and must be < count?
    // Pascal: fileread(f, Count, 4); ... fileseek(f, (num + 1) * 4, 0);
    // If num >= Count, we might be reading past the index table.
    // But typically Count is the number of images.
    
    // Safety check?
    // If num is very large, seeking might fail.
    
    int32_t endOffset = 0;
    int32_t startOffset = 0;

    // Read End Offset (at (num + 1) * 4)
    file.seekg((num + 1) * 4, std::ios::beg);
    if (file.fail()) {
        std::cerr << "PicLoader: Invalid index " << num << " for file " << path << std::endl;
        return result;
    }
    file.read(reinterpret_cast<char*>(&endOffset), 4);

    if (num == 0) {
        startOffset = (count + 1) * 4;
    } else {
        // Read Start Offset (at num * 4)
        file.seekg(num * 4, std::ios::beg);
        file.read(reinterpret_cast<char*>(&startOffset), 4);
    }

    // Calculate Data Length (minus 12 byte header)
    int32_t dataLen = endOffset - startOffset - 12;
    if (dataLen <= 0) {
        // Empty or invalid image data
        return result;
    }

    // Seek to Start Offset
    file.seekg(startOffset, std::ios::beg);
    if (file.fail()) {
         std::cerr << "PicLoader: Invalid data offset for index " << num << std::endl;
         return result;
    }

    // Read Header (x, y, black)
    int32_t x = 0, y = 0, black = 0;
    file.read(reinterpret_cast<char*>(&x), 4);
    file.read(reinterpret_cast<char*>(&y), 4);
    file.read(reinterpret_cast<char*>(&black), 4);
    
    result.x = x;
    result.y = y;
    result.black = black;

    // Read Image Data
    std::vector<uint8_t> buffer(dataLen);
    file.read(reinterpret_cast<char*>(buffer.data()), dataLen);
    if (file.gcount() != dataLen) {
        std::cerr << "PicLoader: Failed to read full image data" << std::endl;
        return result;
    }

    // Load Image from Memory
    SDL_IOStream* io = SDL_IOFromMem(buffer.data(), dataLen);
    if (io) {
        result.surface = IMG_Load_IO(io, true); // true = closes IO automatically
        if (!result.surface) {
             std::cerr << "PicLoader: IMG_Load_IO failed: " << SDL_GetError() << std::endl;
        }
    } else {
        std::cerr << "PicLoader: SDL_IOFromMem failed" << std::endl;
    }

    return result;
}

void PicLoader::freePic(PicImage& pic) {
    if (pic.surface) {
        SDL_DestroySurface(pic.surface);
        pic.surface = nullptr;
    }
}

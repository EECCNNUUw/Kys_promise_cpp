#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <iostream>

// 文件加载辅助类 - 处理基础 IO 操作
// 对应 Pascal 中的 AssignFile, Reset, BlockRead 等操作
class FileLoader {
public:
    // 读取整个文件到内存
    static std::vector<uint8_t> loadFile(const std::string& filename);

    // 从 Group 包 (.grp) 读取特定索引的记录
    // 配合 Index 文件 (.idx) 使用
    // .idx 文件通常存储 4字节的偏移量列表
    // 记录 i 的大小 = offset[i+1] - offset[i]
    // 对应 Pascal 中的资源读取逻辑
    static std::vector<uint8_t> loadGroupRecord(const std::string& grpPath, const std::string& idxPath, int index);

    // 获取资源路径前缀 (如 "resource/")
    static std::string getResourcePath(const std::string& filename);

    // 将内存数据写入文件
    // 对应 Pascal: Rewrite, BlockWrite
    static bool saveFile(const std::string& filename, const void* data, size_t size);
};

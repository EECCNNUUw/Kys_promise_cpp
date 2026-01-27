#pragma once
#include "GameTypes.h"
#include <vector>
#include <cstring>
#include <algorithm>

// Base class for game entities that need to maintain binary compatibility
// 游戏实体基类：旨在保持与Pascal原版二进制数据的完全兼容
// 原版KYS使用结构体直接映射内存（.grp文件），为了能直接读写原版存档，
// 我们使用 int16 数组 (m_data) 来存储所有数据。
// 所有派生类 (Role, Item, Magic, Scene) 都不应添加新的成员变量用于存储游戏状态，
// 而应通过 getter/setter 操作 m_data。
class GameObject {
public:
    GameObject(size_t dataSize) : m_data(dataSize, 0) {}
    virtual ~GameObject() = default;

    // Direct access for serialization
    // 直接访问原始数据，用于文件读写
    int16* getRawData() { return m_data.data(); }
    const int16* getRawData() const { return m_data.data(); }
    size_t getDataSize() const { return m_data.size(); }

    void setDataVector(const std::vector<int16>& data) {
        if (data.size() == m_data.size()) {
            m_data = data;
        } else if (data.size() < m_data.size()) {
            std::copy(data.begin(), data.end(), m_data.begin());
        } else {
            m_data = data; // Or resize? Usually we want fixed size.
        }
    }

    // 从缓冲区加载数据
    // 对应Pascal中的 BlockRead
    void loadFromBuffer(const int16* buffer, size_t size) {
        if (size <= m_data.size()) {
            std::memcpy(m_data.data(), buffer, size * sizeof(int16));
        }
    }

protected:
    // 核心数据存储
    // Pascal中的 integer/smallint 对应 int16_t (2字节)
    // 即使是字符串 (char array) 也是存储在这块内存中
    std::vector<int16> m_data;

    int16 getData(size_t index) const {
        return (index < m_data.size()) ? m_data[index] : 0;
    }

    void setData(size_t index, int16 value) {
        if (index < m_data.size()) {
            m_data[index] = value;
        }
    }

    // Helper to get string from int16 array
    // 辅助函数：从 int16 数组中提取字符串
    // Pascal 中的字符串通常定义为 array[0..N] of char
    // 在内存中占据 N+1 字节。这里将其转换为 C++ std::string。
    std::string getString(size_t offset, size_t length) const {
        std::string s;
        const char* ptr = reinterpret_cast<const char*>(&m_data[offset]);
        // Length in bytes = length * 2 (since int16)
        // Pascal定义通常是 char 数组，但在 int16 数组中，每元素占2字节。
        // 我们需要读取指定长度（以 int16 为单位）内的所有字节，直到遇到 \0
        for (size_t i = 0; i < length * 2; ++i) {
            if (ptr[i] == 0) break;
            s += ptr[i];
        }
        return s;
    }

    // Helper to set string to int16 array
    // 辅助函数：将 C++ std::string 写入 int16 数组
    void setString(size_t offset, size_t length, const std::string& val) {
        char* ptr = reinterpret_cast<char*>(&m_data[offset]);
        std::memset(ptr, 0, length * 2);
        std::strncpy(ptr, val.c_str(), length * 2 - 1);
    }
};

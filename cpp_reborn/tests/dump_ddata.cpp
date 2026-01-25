#include "FileLoader.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>

struct EventData {
    int16_t data[11]; // 0-10
};

struct SceneEventData {
    EventData data[200];
};

int main() {
    std::cout << "Dumping Scene 0 Event Data..." << std::endl;

    // Load alldef.grp (DData)
    // Assuming size is 200 events * 22 bytes * scenes
    // But SceneManager says LoadEventData loads specific amount.
    // Let's use FileLoader.
    
    auto dData = FileLoader::loadFile("save/D1.grp"); // Wait, save file? Or original?
    // Game uses alldef.grp usually. Or D*.grp for save.
    // Let's try alldef.grp first.
    
    // Actually, SceneManager loads `alldef.grp` but also `D*.grp` for saves.
    // Let's check `alldef.grp` (Initial State).
    
    // Hardcode absolute path for testing
    auto grpData = FileLoader::loadFile("e:/Github/kys-promise-main/cpp_reborn/build_win/save/alldef.grp");
    
    if (grpData.empty()) {
        std::cerr << "Failed to load alldef.grp from absolute path" << std::endl;
        return 1;
    }
    
    // Scene 0 is at offset 0
    // Size of one scene: 200 * 11 * 2 = 4400 bytes
    size_t sceneSize = 4400;
    
    if (grpData.size() < sceneSize) {
        std::cerr << "alldef.grp too small" << std::endl;
        return 1;
    }
    
    // Cast to int16
    const int16_t* ptr = reinterpret_cast<const int16_t*>(grpData.data());
    
    // Scene 0, Event 3
    // Offset = 0 + 3 * 11 = 33
    int eventIndex = 3;
    const int16_t* eventPtr = ptr + (eventIndex * 11);
    
    std::cout << "Scene 0 Event " << eventIndex << " Data:" << std::endl;
    for (int k = 0; k < 11; ++k) {
        std::cout << "Index " << k << ": " << eventPtr[k] << std::endl;
    }
    
    std::cout << "Pic Index (Index 5): " << eventPtr[5] << std::endl;

    // Check Event 0 (Bed/Hero)
    eventIndex = 0;
    eventPtr = ptr + (eventIndex * 11);
    std::cout << "\nScene 0 Event " << eventIndex << " Data:" << std::endl;
    std::cout << "Pic Index (Index 5): " << eventPtr[5] << std::endl;

    return 0;
}

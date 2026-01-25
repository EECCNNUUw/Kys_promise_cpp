#include <iostream>
#include "EventManager.h"
#include "GameManager.h"
#include "SceneManager.h"
#include "FileLoader.h"

int main() {
    std::cout << "=== Testing Scene Trigger Logic ===" << std::endl;
    
    // 1. Init (Minimal)
    // We skip full GameManager::Init because it creates Window/Renderer which might fail in some envs
    // But we need FileLoader and SceneManager initialized.
    
    // Manually load data
    std::cout << "Loading data from ../save/" << std::endl;
    
    if (!SceneManager::getInstance().LoadEventData("../save/alldef.grp")) {
         std::cerr << "Failed to load alldef.grp. Test might fail." << std::endl;
    }
    if (!SceneManager::getInstance().LoadMapData("../save/allsin.grp")) {
         std::cerr << "Failed to load allsin.grp. Test might fail." << std::endl;
    }

    // 2. Setup Test Data
    int testSceneId = 0;
    int testX = 10;
    int testY = 10;
    int testEventIndex = 1; // Use Event 1
    int testScriptId = 9999; // Use a high ID that likely doesn't exist but triggers "Invalid ID" or "Out of bounds" log
    
    // Inject Event at (10, 10) in Layer 3
    SceneManager::getInstance().SetSceneTile(testSceneId, 3, testX, testY, testEventIndex);
    
    // Set Script ID for Event 1 to 9999
    // Index 4 is Script ID
    SceneManager::getInstance().SetEventData(testSceneId, testEventIndex, 4, testScriptId);
    
    std::cout << "Injecting Event " << testEventIndex << " at (" << testX << "," << testY << ") with Script " << testScriptId << std::endl;
    
    // Verify Injection
    int16_t val = SceneManager::getInstance().GetSceneTile(testSceneId, 3, testX, testY);
    std::cout << "Read back Layer 3 at (" << testX << "," << testY << "): " << val << std::endl;
    
    int16_t script = SceneManager::getInstance().GetEventData(testSceneId, testEventIndex, 4);
    std::cout << "Read back Script ID for Event " << testEventIndex << ": " << script << std::endl;

    // 3. Call Instruct_JmpScene
    std::cout << "Calling Instruct_JmpScene(" << testSceneId << ", " << testX << ", " << testY << ")..." << std::endl;
    
    // Note: Instruct_JmpScene calls GameManager functions which might rely on SDL
    // But we didn't init SDL.
    // GameManager::setMainMapPosition just sets ints.
    // Instruct_Redraw calls UIManager::UpdateScreen which might crash if no Renderer.
    // But we can try.
    
    try {
        EventManager::getInstance().Instruct_JmpScene(testSceneId, testX, testY);
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown Exception (likely SDL nullptr access)" << std::endl;
    }
    
    std::cout << "=== Test Finished ===" << std::endl;
    return 0;
}

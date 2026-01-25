#include <iostream>
#include <vector>
#include "EventManager.h"
#include "GameManager.h"

// Simple mock for testing without full graphics
int main() {
    std::cout << "=== Testing Event 101 Execution ===" << std::endl;

    // 1. Initialize EventManager (Load Scripts)
    if (!EventManager::getInstance().LoadScripts()) {
        std::cerr << "[FAIL] Failed to load kdef scripts" << std::endl;
        return 1;
    }
    std::cout << "[PASS] Scripts loaded." << std::endl;

    // 2. Initialize Dialogues
    if (!EventManager::getInstance().LoadDialogues()) {
        std::cerr << "[FAIL] Failed to load dialogues" << std::endl;
        return 1;
    }
    std::cout << "[PASS] Dialogues loaded." << std::endl;

    // 3. Simulate Event 101 execution
    // Note: This will try to run real instructions. 
    // Since we are not running the full game loop (SDL), 
    // UI calls like ShowChoice might block or fail if they rely on Renderer.
    // However, we just want to see the Opcode sequence in the console log 
    // which we enabled in EventManager.cpp.
    
    std::cout << "\n--- Simulating ExecuteEvent(101) ---" << std::endl;
    std::cout << "Check console output for Opcode sequence." << std::endl;
    std::cout << "Expected: Opcode 4 (Check Item) -> ... -> Opcode 39 (Change Scene)" << std::endl;
    std::cout << "If you see Opcode 5 (Ask Fight) immediately, it's wrong." << std::endl;
    
    // We need to bypass the actual execution if it blocks on UI.
    // But we modified ExecuteEvent to print Opcodes.
    // Let's run it. If it crashes on UI, we at least see the log before.
    
    try {
        EventManager::getInstance().ExecuteEvent(101);
    } catch (...) {
        std::cerr << "[CRASH] Exception during execution (expected if UI missing)" << std::endl;
    }
    
    std::cout << "\n=== Test Finished ===" << std::endl;
    return 0;
}

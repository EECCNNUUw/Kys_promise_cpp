#include <iostream>
#include <fstream>
#include <cassert>
#include "GameManager.h"
#include "Role.h"
#include "Item.h"

// Helper to check if file exists
bool FileExists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

int main() {
    std::cout << "=== Test Save/Load ===" << std::endl;

    GameManager& gm = GameManager::getInstance();
    // Do NOT call gm.Init() to avoid SDL dependency in this data test

    // 1. Setup initial state
    Role testRole;
    // testRole.setID(0); // ID is index in vector
    testRole.setName("TestHero");
    testRole.setMaxHP(100);
    testRole.setCurrentHP(50);
    
    Item testItem;
    testItem.setName("TestPotion");
    testItem.setPrice(100); // Using setPrice as a proxy for data
    
    gm.clearDataForTest();
    gm.addRoleForTest(testRole);
    gm.addItemForTest(testItem);
    
    // We need to ensure ranger.idx exists because SaveGame reads it to get offsets!
    if (!FileExists("save/ranger.idx") && !FileExists("../save/ranger.idx")) {
        std::cerr << "[SKIP] ranger.idx not found. Cannot run test." << std::endl;
        return 0; // Skip test
    }
    
    gm.m_savePath = "save/";
    
    // 2. Save Game (Slot 1 -> R1.grp)
    std::cout << "Saving to slot 1..." << std::endl;
    gm.SaveGame(1);
    
    if (FileExists("save/R1.grp")) {
        std::cout << "[PASS] save/R1.grp created." << std::endl;
    } else {
        std::cout << "[FAIL] save/R1.grp NOT created." << std::endl;
        return 1;
    }
    
    // 3. Modify Data
    std::cout << "Modifying data..." << std::endl;
    gm.getRole(0).setCurrentHP(10);
    gm.getItem(0).setPrice(999);
    assert(gm.getRole(0).getCurrentHP() == 10);
    assert(gm.getItem(0).getPrice() == 999);
    
    // 4. Load Game
    std::cout << "Loading from slot 1..." << std::endl;
    gm.LoadGame(1);
    
    // 5. Verify
    int hp = gm.getRole(0).getCurrentHP();
    int price = gm.getItem(0).getPrice();
    
    std::cout << "Role 0 HP after load: " << hp << std::endl;
    std::cout << "Item 0 Price after load: " << price << std::endl;
    
    bool pass = true;
    if (hp == 50) {
        std::cout << "[PASS] Role Data restored correctly." << std::endl;
    } else {
        std::cout << "[FAIL] Role Data NOT restored. Expected 50, got " << hp << std::endl;
        pass = false;
    }
    
    if (price == 100) {
        std::cout << "[PASS] Item Data restored correctly." << std::endl;
    } else {
        std::cout << "[FAIL] Item Data NOT restored. Expected 100, got " << price << std::endl;
        pass = false;
    }
    
    if (!pass) return 1;
    
    std::cout << "=== Test Finished ===" << std::endl;
    return 0;
}

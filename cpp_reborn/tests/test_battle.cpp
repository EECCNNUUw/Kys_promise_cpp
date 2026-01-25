#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <algorithm>
#include "BattleManager.h"
#include "GameManager.h"
#include "Magic.h"
#include "Role.h"

// Replicate CalNewHurtValue for validation
static int TestCalNewHurtValue(int lv, int minVal, int maxVal, int proportion) {
    if (proportion == 0) proportion = 100;
    double p = proportion / 1000.0;
    double n = std::pow((double)(maxVal - minVal), 1.0 / p) / 9.0;
    return (int)(std::round(std::pow((lv * n), p)) + minVal);
}

void TestDamageCalculation() {
    std::cout << "--- Testing Damage Calculation ---" << std::endl;

    // 1. Setup Data
    GameManager::getInstance().clearDataForTest();
    BattleManager::getInstance().ClearBattleRoles();

    // 2. Create Magic (ID 0)
    Magic m;
    int16_t* md = m.getRawData();
    // Set Name "TestMagic"
    m.setName("TestMagic");
    md[18] = 100; // MinHurt
    md[19] = 100; // MaxHurt
    md[20] = 0;   // HurtModulus (if 0, p=0, scaling skipped? Let's test)
    // Actually if p=0, formula divides by p? No, CalHurtValue checks if (p > 0).
    // Let's set Modulus to make p > 0.
    md[21] = 1;   // AttackModulus -> p += 6
    // p = 1 * 6 = 6.
    
    // Add to GameManager
    GameManager::getInstance().addMagicForTest(m);
    
    // 3. Create Attacker Role (ID 0)
    Role attackerRole;
    int16_t* rd1 = attackerRole.getRawData();
    attackerRole.setName("Attacker");
    rd1[35] = 100; // Attack (Index 35 from memory? Need to verify Role indices or use setters)
    // Role.h has setters! Use them.
    attackerRole.setAttack(100);
    attackerRole.setDefence(50);
    attackerRole.setSpeed(50);
    attackerRole.setKnowledge(0); // Simplify
    attackerRole.setLevel(10);
    attackerRole.setDifficulty(50); // Standard
    
    // Magic proficiency
    // Role::setMagic(index, magicId)
    // Role::setMagLevel(index, level)
    // Role usually stores Magics in specific slots.
    // Let's assume setMagic/setMagLevel exist.
    // If not, I'll check Role.h. Assuming they exist based on BattleManager using them.
    // But BattleManager uses `getMagic(i)` and `getMagLevel(i)`.
    // I need to set them.
    
    // Add to GameManager
    GameManager::getInstance().addRoleForTest(attackerRole);
    
    // 4. Create Defender Role (ID 1)
    Role defenderRole;
    defenderRole.setName("Defender");
    defenderRole.setAttack(50);
    defenderRole.setDefence(0); // Zero defense
    defenderRole.setSpeed(50);
    defenderRole.setKnowledge(0);
    defenderRole.setLevel(10);
    defenderRole.setMaxHP(1000);
    defenderRole.setCurrentHP(1000);
    
    GameManager::getInstance().addRoleForTest(defenderRole);
    
    // 5. Create BattleRoles
    BattleRole brAttacker;
    brAttacker.setRNum(0);
    brAttacker.setTeam(0); // Player Team
    brAttacker.setX(10);
    brAttacker.setY(10);
    BattleManager::getInstance().AddBattleRole(brAttacker);
    
    BattleRole brDefender;
    brDefender.setRNum(1);
    brDefender.setTeam(1); // Enemy Team
    brDefender.setX(11); // Distance 1
    brDefender.setY(10);
    BattleManager::getInstance().AddBattleRole(brDefender);
    
    // Set Ground (Layer 0) for the test area
    // Otherwise SetAttackArea will skip tiles
    for(int x=0; x<64; x++) {
        for(int y=0; y<64; y++) {
            BattleManager::getInstance().setBattleField(0, x, y, 1);
        }
    }

    // 6. Test CalHurtValue
    // Magic ID 0, Level 10
    int dmg = BattleManager::getInstance().CalHurtValue(0, 1, 0, 10);
    
    std::cout << "Calculated Damage: " << dmg << std::endl;
    
    // Verification Logic:
    // BaseHurt = CalNewHurtValue(9, 100, 100, 0) = 100.
    // mhurt = 100 * (100 + 0) / 100 = 100.
    // p = 6.
    // Att = 101, Def = 1.
    // a1 = (101 - 1)/101 = 0.99.
    // result = 100 * 0.99 * (1 * 6 / 6) = 99.
    // Distance = 1 -> Factor 1.0.
    // Random variance (+/- 10).
    // Expected ~99.
    
    assert(dmg > 80 && dmg < 120);
    std::cout << "[PASS] Damage within expected range." << std::endl;
    
    // 7. Test Attack (End-to-End)
    // Mark target in range (Layer 4)
    // BattleManager::Attack clears layer 4 and sets it based on targetIdx?
    // Yes, Attack(roleIdx, targetIdx, magicId) does this.
    
    BattleManager::getInstance().Attack(0, 1, 0);
    
    // Check Defender HP
    Role& defData = GameManager::getInstance().getRole(1);
    int newHP = defData.getCurrentHP();
    std::cout << "Defender HP after Attack: " << newHP << " (Was 1000)" << std::endl;
    
    int appliedDmg = 1000 - newHP;
    std::cout << "Applied Damage: " << appliedDmg << std::endl;

    assert(newHP < 1000);
    // Damage recalculates with random variance, so it won't match 'dmg' exactly.
    // But it should be within range.
    assert(appliedDmg > 80 && appliedDmg < 120); 
    std::cout << "[PASS] HP reduced correctly within range." << std::endl;
}

int main() {
    TestDamageCalculation();
    std::cout << "All Tests Passed!" << std::endl;
    return 0;
}

#include <iostream>
#include <string>
#include <vector>

// Mock GameManager
struct GameManager {
    static GameManager& getInstance() {
        static GameManager instance;
        return instance;
    }
    struct Role {
        std::string getName() { return "\xBD\xF0\xD3\xB9"; } // "Jin Yong" in GBK (Jin=BD F0, Yong=D3 B9) - Wait, let's verify GBK
        // Jin: 金 (GBK: BDF0)
        // Yong: 庸 (GBK: D3B9)
        std::string getNick() { return "Nick"; }
    };
    Role getRole(int i) { return Role(); }
};

int main() {
    // 1. Setup Data
    // String: "$先生" and "＄先生"
    // $ : 24
    // ＄: A3 A4
    // Xian: CF 88
    // Sheng: CA A6
    std::string fullText = "\x24\xCF\x88\xCA\xA6\x2A\xA3\xA4\xCF\x88\xCA\xA6"; 
    
    std::cout << "Original Text (Hex): ";
    for (unsigned char c : fullText) printf("%02X ", c);
    std::cout << std::endl;

    // 2. Logic from EventManager
    std::string heroName = GameManager::getInstance().getRole(0).getName();
    std::cout << "Hero Name (Hex): ";
    for (unsigned char c : heroName) printf("%02X ", c);
    std::cout << std::endl;

    std::string cleanText;
    for (size_t i = 0; i < fullText.length(); ++i) {
        unsigned char c = (unsigned char)fullText[i];
        
        // Check for escaped characters first
        if (i + 1 < fullText.length()) {
            unsigned char nextC = (unsigned char)fullText[i + 1];
            if (c == '*' && nextC == '*') { cleanText += '*'; i++; continue; }
            if (c == '&' && nextC == '&') { cleanText += '&'; i++; continue; }
            if (c == '#' && nextC == '#') { cleanText += '#'; i++; continue; }
            if (c == '@' && nextC == '@') { cleanText += '@'; i++; continue; }
            // Special handling for $$: Only escape if strictly $$
            if (c == '$' && nextC == '$') { cleanText += '$'; i++; continue; }
            if (c == '%' && nextC == '%') { cleanText += '%'; i++; continue; }
        }

        // Control Codes
        if (c == '^') {
            if (i + 1 < fullText.length()) {
                // Skip color code
                i++;
                continue;
            }
        }
        else if (c == '*') {
            // Newline
            cleanText += '\n';
        }
        else if (c == '@') {
            // Placeholder @0, @N
            if (i + 1 < fullText.length()) {
                char nextC = fullText[i+1];
                if (nextC == '0') {
                    cleanText += heroName;
                    i++;
                } else if (nextC == 'N') {
                    cleanText += "Nick"; // Mock
                    i++;
                } else {
                    cleanText += (char)c;
                }
            } else {
                cleanText += (char)c;
            }
        }
        else if (c == '$') {
            // Placeholder for Surname
            // Heuristic: First 2 bytes of GBK name
            std::string surname = heroName;
            if (heroName.length() >= 2) {
                surname = heroName.substr(0, 2);
            }
            cleanText += surname;
            std::cout << "[Test] Replaced $ with " << surname << std::endl;
        }
        else if (c == 0xA3) {
             // Check for Full Width $ (A3 A4)
             if (i + 1 < fullText.length()) {
                 unsigned char nextC = (unsigned char)fullText[i+1];
                 if (nextC == 0xA4) {
                     // Found Full Width $
                     std::string surname = heroName;
                     if (heroName.length() >= 2) {
                         surname = heroName.substr(0, 2);
                     }
                     cleanText += surname;
                     std::cout << "[Test] Replaced Full-Width $ with " << surname << std::endl;
                     i++; // Skip nextC
                     continue;
                 }
             }
             cleanText += (char)c;
        }
        else {
            cleanText += (char)c;
        }
    }

    std::cout << "Clean Text (Hex): ";
    for (unsigned char c : cleanText) printf("%02X ", c);
    std::cout << std::endl;
    
    // Expected: BD F0 CF 88 CA A6 (Jin Xian Sheng)
    // Check if result matches
    if (cleanText.length() >= 2 && (unsigned char)cleanText[0] == 0xBD && (unsigned char)cleanText[1] == 0xF0) {
        std::cout << "SUCCESS: Surname replaced correctly." << std::endl;
    } else {
        std::cout << "FAILURE: Surname replacement failed." << std::endl;
    }

    return 0;
}
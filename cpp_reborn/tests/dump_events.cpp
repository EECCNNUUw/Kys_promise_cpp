#include "EventManager.h"
#include "FileLoader.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include <cstring>

int main() {
    std::cout << "Scanning events for Pan(25) -> NewTalk0(68) pattern..." << std::endl;

    // Load kdef.idx and kdef.grp
    // Try multiple paths
    std::string paths[] = {
        "kdef.idx", 
        "../resource/kdef.idx", 
        "../../resource/kdef.idx",
        "d:/program/misc/kys-promise-main/resource/kdef.idx"
    };
    
    std::vector<uint8_t> idxData, grpData;
    
    for (const auto& p : paths) {
        idxData = FileLoader::loadFile(p);
        if (!idxData.empty()) {
            std::cout << "Loaded idx from " << p << std::endl;
            std::string grpPath = p;
            grpPath.replace(grpPath.find(".idx"), 4, ".grp");
            grpData = FileLoader::loadFile(grpPath);
            break;
        }
    }

    if (idxData.empty() || grpData.empty()) {
        std::cerr << "Failed to load kdef files." << std::endl;
        return 1;
    }

    std::vector<int16_t> eventScripts(grpData.size() / 2);
    std::memcpy(eventScripts.data(), grpData.data(), grpData.size());

    std::vector<int32_t> eventIndices(idxData.size() / 4);
    std::memcpy(eventIndices.data(), idxData.data(), idxData.size());

    int eventCount = eventIndices.size();
    std::cout << "Total Events: " << eventCount << std::endl;

    // Dump Event 101
    int eventId = 101;
    if (eventId > 0 && eventId <= eventCount) {
        int offset = eventIndices[eventId - 1];
        int nextOffset = (eventId < eventCount) ? eventIndices[eventId] : eventScripts.size() * 2;
        int len = (nextOffset - offset) / 2;
        int start = offset / 2;
        
        std::cout << "\n--- Dumping Event " << eventId << " ---\n";
        int pc = start;
        int end = start + len;
        
        while (pc < end) {
            int16_t op = eventScripts[pc++];
            std::cout << "PC " << (pc-1) << ": Op " << op;
            
            // Decode args roughly
            if (op == 50) { // PlaySound
                 int arg = eventScripts[pc++];
                 std::cout << " (PlaySound " << arg << ")";
            }
            else if (op == 1) { // Dialogue
                int talkId = eventScripts[pc++];
                int headId = eventScripts[pc++];
                int mode = eventScripts[pc++];
                std::cout << " (Dialogue TalkID=" << talkId << " Head=" << headId << " Mode=" << mode << ")";
            }
            else if (op == 68) { // NewTalk0
                std::cout << " (NewTalk0 args: ";
                for(int k=0; k<7; ++k) std::cout << eventScripts[pc++] << " ";
                std::cout << ")";
            }
            else if (op == 3) { // ModifyEvent
                std::cout << " (ModifyEvent args: ";
                for(int k=0; k<13; ++k) std::cout << eventScripts[pc++] << " ";
                std::cout << ")";
            }
            else if (op == 25) { // Instruct_25(x1, y1, x2, y2)
                 int x1 = eventScripts[pc++];
                 int y1 = eventScripts[pc++];
                 int x2 = eventScripts[pc++];
                 int y2 = eventScripts[pc++];
                 std::cout << " (Pan Screen: " << x1 << "," << y1 << " -> " << x2 << "," << y2 << ")";
            }
            else if (op == 0) {
                 std::cout << " (Redraw/End)";
                 // Check next
                 if (pc < end) {
                     int16_t next = eventScripts[pc];
                     std::cout << " [Next: " << next << "]";
                 }
            }
            // Add more if needed
            
            std::cout << std::endl;
            
            if (op < 0) {
                std::cout << "WARNING: Negative Opcode " << op << std::endl;
            }
        }
    }
    
    // Assume Script ID is something?
    // Or scan all events for one that uses "Jin Xiansheng" dialogue?
    // Jin Xiansheng name? "金先生".
    // I can't search for Chinese string easily without encoding.
    
    // Let's dump all events that look like "Jin Xiansheng" (Dialogue with specific HeadID?)
    // Need to know Jin Xiansheng's HeadID.
    
    // Instead, I will implement a "CheckAutoEvents" simulation.
    // Iterate all events in Scene 52 (if I had DData).
    
    // Check for Event 2235 logic
    std::vector<int> checkEvents = {2235, 2234};
    
    for (int eventId : checkEvents) {
        if (eventId > eventIndices.size()) continue;

        int offset = eventIndices[eventId - 1];
        int nextOffset = (eventId < eventIndices.size()) ? eventIndices[eventId] : eventScripts.size() * 2;
        int lenWords = (nextOffset - offset) / 2;
        int start = offset / 2;

        std::cout << "\nEvent " << eventId << " (Length: " << lenWords << "):" << std::endl;
        
        for (int i = 0; i < lenWords; ++i) {
            int16_t opcode = eventScripts[start + i];
            std::cout << "[" << i << "] Op: " << opcode;

            if (opcode == 68) { // NewTalk0
                std::cout << " (NewTalk0) Args: ";
                for (int k = 1; k <= 7; ++k) std::cout << eventScripts[start + i + k] << " ";
                i += 7; 
            } else if (opcode == 1) { // Dialogue
                std::cout << " (Dialogue) Args: ";
                for (int k = 1; k <= 3; ++k) std::cout << eventScripts[start + i + k] << " ";
                i += 3;
            } else if (opcode == 3) { // ModEvent
                std::cout << " (ModEvent) Args: ";
                // Check if this modifies Scene 52 Event 1
                int s = eventScripts[start + i + 1];
                int e = eventScripts[start + i + 2];
                int cond = eventScripts[start + i + 3];
                std::cout << "S=" << s << " E=" << e << " Cond=" << cond << " ";
                
                for (int k = 1; k <= 13; ++k) std::cout << eventScripts[start + i + k] << " ";
                i += 13;
            } else if (opcode == 25) { // Pan
                int x1 = eventScripts[start + i + 1];
                int y1 = eventScripts[start + i + 2];
                int x2 = eventScripts[start + i + 3];
                int y2 = eventScripts[start + i + 4];
                std::cout << " (Pan) Args: " << x1 << " " << y1 << " " << x2 << " " << y2;
                i += 4;
            } else if (opcode == 0) {
                std::cout << " (Redraw/Exit?)";
            }
            
            std::cout << std::endl;
        }
    }

    return 0;
}

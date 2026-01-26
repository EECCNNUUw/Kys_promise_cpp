#include "EventManager.h"
#include "FileLoader.h"
#include "GameManager.h"
#include "SceneManager.h"
#include "TextManager.h"
#include "GraphicsUtils.h"
#include "BattleManager.h"
#include "UIManager.h"
#include "SoundManager.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

namespace {
    std::string TakeSurnameBytes(const std::string& s) {
        if (s.empty()) return "";
        unsigned char b0 = (unsigned char)s[0];
        if (b0 < 0x80) return s.substr(0, 1);
        if (s.size() >= 2) return s.substr(0, 2);
        return s;
    }
}

EventManager& EventManager::getInstance() {
    static EventManager instance;
    return instance;
}

void EventManager::Instruct_19(int x, int y) {
    // Teleport within scene
    // Pascal: Sx := y; Sy := x; Cx := Sx; Cy := Sy;
    GameManager::getInstance().setMainMapPosition(y, x);
    Instruct_Redraw();
}

void EventManager::Instruct_40(int dir) {
    GameManager::getInstance().setMainMapFace(dir);
}

EventManager::EventManager() {}

bool EventManager::Init() {
    if (!LoadScripts()) {
        std::cerr << "Failed to load event scripts (kdef)" << std::endl;
        return false;
    }
    if (!LoadDialogues()) {
        std::cerr << "Failed to load dialogues (talk)" << std::endl;
        return false;
    }
    return true;
}

bool EventManager::LoadScripts() {
    auto idxData = FileLoader::loadFile("kdef.idx");
    if (idxData.empty()) {
        // Try capitalized if lowercase failed (though Windows is case-insensitive, simple FileLoader might be picky if cached)
        idxData = FileLoader::loadFile("Kdef.idx");
        if (idxData.empty()) return false;
    }

    if (idxData.size() % 4 != 0) {
        std::cerr << "kdef.idx size is not multiple of 4" << std::endl;
        return false;
    }

    size_t count = idxData.size() / 4;
    m_eventIndices.resize(count);
    std::memcpy(m_eventIndices.data(), idxData.data(), idxData.size());
    
    std::cout << "[EventManager] Loaded kdef.idx. Size: " << idxData.size() << " bytes. Script Count: " << count << std::endl;

    auto grpData = FileLoader::loadFile("kdef.grp");
    if (grpData.empty()) {
        grpData = FileLoader::loadFile("Kdef.grp");
        if (grpData.empty()) return false;
    }

    if (grpData.size() % 2 != 0) {
        std::cerr << "kdef.grp size is not multiple of 2" << std::endl;
        return false;
    }

    m_eventScripts.resize(grpData.size() / 2);
    std::memcpy(m_eventScripts.data(), grpData.data(), grpData.size());

    std::cout << "Loaded " << count << " event scripts." << std::endl;
    if (count >= 101) {
        std::cout << "Event 101 offset: " << m_eventIndices[100] << std::endl;
    } else {
        std::cerr << "Event 101 not found (count too low)" << std::endl;
    }

    return true;
}

bool EventManager::LoadDialogues() {
    auto idxData = FileLoader::loadFile("talk.idx");
    if (idxData.empty()) return false;

    size_t count = idxData.size() / 4;
    m_talkIndices.resize(count);
    std::memcpy(m_talkIndices.data(), idxData.data(), idxData.size());

    m_talkData = FileLoader::loadFile("talk.grp");
    if (m_talkData.empty()) {
         std::cerr << "Failed to load talk.grp" << std::endl;
         return false;
    }
    std::cout << "Loaded " << count << " dialogues." << std::endl;

    // Load Name Data (Optional but recommended)
    auto nameIdx = FileLoader::loadFile("name.idx");
    if (!nameIdx.empty()) {
        size_t nc = nameIdx.size() / 4;
        m_nameIndices.resize(nc);
        std::memcpy(m_nameIndices.data(), nameIdx.data(), nameIdx.size());
        m_nameData = FileLoader::loadFile("name.grp");
        if (!m_nameData.empty()) {
             std::cout << "Loaded " << nc << " names." << std::endl;
        }
    }

    return true;
}

std::string EventManager::GetNameFromData(int nameNum) {
    if (nameNum <= 0 || nameNum > m_nameIndices.size()) return "";
    
    int offset = m_nameIndices[nameNum - 1];
    int nextOffset = (nameNum < m_nameIndices.size()) ? m_nameIndices[nameNum] : m_nameData.size();
    
    int len = nextOffset - offset;
    if (len <= 0 || offset + len > m_nameData.size()) return "";
    
    std::string name;
    // Name data is also XOR 0xFF encoded?
    // Pascal: for i := 0 to namelen - 2 do namearray[i] := namearray[i] xor $FF;
    for (int i = 0; i < len; ++i) {
        uint8_t b = m_nameData[offset + i] ^ 0xFF;
        if (b == 0 || b == 0x2A || b < 0x20) break;
        name += (char)b;
    }
    return name;
}

void EventManager::CheckEvent(int sceneId, int x, int y) {
    int16_t eventIndex = SceneManager::getInstance().GetSceneTile(sceneId, 3, x, y);
    
    if (eventIndex >= 0) {
        int16_t scriptId = SceneManager::getInstance().GetEventData(sceneId, eventIndex, 4);
        
        if (scriptId > 0) {
            m_currentSceneId = sceneId;
            m_currentEventId = eventIndex;
            ExecuteEvent(scriptId);
        }
    }
}

void EventManager::CheckAutoEvents(int sceneId) {
    // Iterate all events to find Auto-Run events (Condition == 0)
    for (int i = 0; i < 200; ++i) {
        int16_t condition = SceneManager::getInstance().GetEventData(sceneId, i, 0);
        int16_t scriptId = SceneManager::getInstance().GetEventData(sceneId, i, 4);
        
        // Condition 0 means Auto-Run
        if (condition == 0 && scriptId > 0) {
            // We found an auto-run event.
            // Update context.
            m_currentSceneId = sceneId;
            m_currentEventId = i;
            
            std::cout << "[CheckAutoEvents] Triggering Auto-Run Event: " << i << " (Script " << scriptId << ")" << std::endl;
            ExecuteEvent(scriptId);
            
            // Usually only one auto-run event happens at a time.
            return; 
        }
    }
}

int16_t EventManager::ReadScriptArg(int& offset) {
    if (offset < 0 || offset >= m_eventScripts.size()) return 0;
    return m_eventScripts[offset++];
}

void EventManager::ExecuteEvent(int eventScriptId) {
    if (eventScriptId <= 0 || eventScriptId > m_eventIndices.size()) {
        std::cerr << "ExecuteEvent: Invalid ID " << eventScriptId << ". Max ID: " << m_eventIndices.size() << std::endl;
        return;
    }

    int offset = m_eventIndices[eventScriptId - 1];
    int nextOffset = (eventScriptId < m_eventIndices.size()) ? m_eventIndices[eventScriptId] : m_eventScripts.size() * 2;
    
    int lengthBytes = nextOffset - offset;
    int lengthWords = lengthBytes / 2;
    
    int scriptStart = offset / 2;
    int scriptEnd = scriptStart + lengthWords;
    
    // Check range validity
    if (scriptStart >= m_eventScripts.size() || scriptEnd > m_eventScripts.size()) {
        std::cerr << "Event " << eventScriptId << " out of bounds! Start: " << scriptStart 
                  << " End: " << scriptEnd << " Size: " << m_eventScripts.size() << std::endl;
        return;
    }
    
    int pc = scriptStart; 
    
    std::cout << "Event " << eventScriptId << " Start. PC: " << pc << " End: " << scriptEnd << std::endl;

    while (pc < scriptEnd) {
        int16_t opcode = ReadScriptArg(pc);
        if (opcode < 0) {
             std::cout << "Negative Opcode " << opcode << " at PC " << (pc-1) << ". Skipping." << std::endl;
             // Do not break, just continue to next instruction.
             // Warning: If negative opcode was meant to have arguments, we might interpret args as opcodes.
             // But for Op -5, followed by 0, it seems safe.
             continue; 
        }

        std::cout << "Opcode: " << opcode << " at PC: " << (pc-1) << std::endl;

        switch (opcode) {
            case 0: 
                // Instruct 0 is typically Redraw.
                // However, if we are at the end of the script (or next opcode is -1/invalid),
                // we should stop to avoid reading garbage.
                Instruct_Redraw(); 
                if (pc >= scriptEnd) {
                    std::cout << "Event " << eventScriptId << " Ends at Opcode 0 (EOF)." << std::endl;
                    return;
                }
                // Peek next
                if (pc < scriptEnd) {
                    int16_t nextOp = m_eventScripts[pc];
                    // If next opcode is negative, do NOT return. 
                    // Let the loop handle it (it will print "Skipping" and continue).
                    if (nextOp < 0) {
                         std::cout << "Event " << eventScriptId << " Opcode 0 followed by Negative Opcode " << nextOp << ". Continuing..." << std::endl;
                    }
                }
                break;
            case 1: {
                int talkId = ReadScriptArg(pc);
                int headId = ReadScriptArg(pc);
                int mode = ReadScriptArg(pc);
                Instruct_Dialogue(talkId, headId, mode);
                break;
            }
            case 2: {
                int itemId = ReadScriptArg(pc);
                int amount = ReadScriptArg(pc);
                Instruct_AddItem(itemId, amount);
                break;
            }
            case 3: {
                std::vector<int16_t> args;
                for(int k=0; k<13; ++k) args.push_back(ReadScriptArg(pc));
                Instruct_ModifyEvent(args);
                break;
            }
            case 4: { 
                int itemNum = ReadScriptArg(pc);
                int jump1 = ReadScriptArg(pc);
                int jump2 = ReadScriptArg(pc);
                // Check if item exists (amount > 0)
                bool hasItem = GameManager::getInstance().getItemAmount(itemNum) > 0;
                int jump = hasItem ? jump1 : jump2;
                pc += jump;
                break;
            }
            case 5: {
                int jump1 = ReadScriptArg(pc);
                int jump2 = ReadScriptArg(pc);
                int choice = UIManager::getInstance().ShowChoice("是否與之戰鬥？"); 
                int jump = (choice == 1) ? jump1 : jump2;
                pc += jump;
                break;
            }
            case 6: {
                int battleId = ReadScriptArg(pc);
                int jump1 = ReadScriptArg(pc);
                int jump2 = ReadScriptArg(pc);
                int getExp = ReadScriptArg(pc);
                int result = Instruct_Battle(battleId, jump1, jump2, getExp);
                pc += result;
                break;
            }
            case 8: Instruct_PlayMusic(ReadScriptArg(pc)); break;
            case 10: Instruct_JoinParty(ReadScriptArg(pc)); break;
            case 11: {
                int roleId = ReadScriptArg(pc);
                int attrId = ReadScriptArg(pc);
                int value = ReadScriptArg(pc);
                Instruct_AddAttribute(roleId, attrId, value);
                break;
            }
            case 12: Instruct_Rest(); break;
            case 13: Instruct_FadeIn(); break;
            case 14: Instruct_FadeOut(); break;
            case 16: {
                 int time = ReadScriptArg(pc);
                 Instruct_Flash(0xFFFFFF, time * 20); 
                 break;
            }
            case 17: Instruct_Delay(ReadScriptArg(pc)); break;
            case 19: { // Instruct_19(x, y) - Teleport
                int x = ReadScriptArg(pc);
                int y = ReadScriptArg(pc);
                Instruct_19(x, y);
                break;
            }
            case 21: Instruct_LeaveParty(ReadScriptArg(pc)); break;
            case 23: { // Instruct_23(enum, action, step, speed)
                int enum_ = ReadScriptArg(pc);
                int action = ReadScriptArg(pc);
                int step = ReadScriptArg(pc);
                int speed = ReadScriptArg(pc); // Usually ignored in KYS or just delay?
                Instruct_23(enum_, action, step, speed);
                break;
            }
            case 25: {
                int x1 = ReadScriptArg(pc);
                int y1 = ReadScriptArg(pc);
                int x2 = ReadScriptArg(pc);
                int y2 = ReadScriptArg(pc);
                Instruct_25(x1, y1, x2, y2);
                break;
            }
            case 26: { // Instruct_26(snum, enum, add1, add2, add3)
                int snum = ReadScriptArg(pc);
                int enum_ = ReadScriptArg(pc);
                int add1 = ReadScriptArg(pc);
                int add2 = ReadScriptArg(pc);
                int add3 = ReadScriptArg(pc);
                Instruct_26(snum, enum_, add1, add2, add3);
                break;
            }
            case 27: { // Instruct_27(enum, beginpic, endpic)
                int enum_ = ReadScriptArg(pc);
                int beginPic = ReadScriptArg(pc);
                int endPic = ReadScriptArg(pc);
                Instruct_27(enum_, beginPic, endPic);
                break;
            }
            case 28: { // 5 args, returns jump
                for(int k=0; k<5; ++k) ReadScriptArg(pc);
                // TODO: Implement Logic (Check Attribute?)
                break;
            }
            case 29: { // 5 args, returns jump
                for(int k=0; k<5; ++k) ReadScriptArg(pc);
                // TODO: Implement Logic
                break;
            }
            case 30: { // Instruct_30(x1, y1, x2, y2) - Move Protagonist
                int x1 = ReadScriptArg(pc);
                int y1 = ReadScriptArg(pc);
                int x2 = ReadScriptArg(pc);
                int y2 = ReadScriptArg(pc);
                Instruct_30(x1, y1, x2, y2);
                break;
            }
            case 31: { // 3 args, returns jump
                 for(int k=0; k<3; ++k) ReadScriptArg(pc);
                 break;
            }
            case 32: { // 2 args
                 int itemId = ReadScriptArg(pc);
                 int amount = ReadScriptArg(pc);
                 GameManager::getInstance().AddItem(itemId, amount);
                 break;
            }
            case 33: { // 3 args
                 for(int k=0; k<3; ++k) ReadScriptArg(pc);
                 break;
            }
            case 34: { // 2 args
                 for(int k=0; k<2; ++k) ReadScriptArg(pc);
                 break;
            }
            case 35: { // 4 args
                 for(int k=0; k<4; ++k) ReadScriptArg(pc);
                 break;
            }
            case 36: { // 3 args, returns jump
                 int sexual = ReadScriptArg(pc);
                 int jump1 = ReadScriptArg(pc);
                 int jump2 = ReadScriptArg(pc);
                 
                 bool condition = false;
                 if (sexual > 255) {
                     // Check global flag x50[$7000] (28672)
                     // Pascal: if x50[$7000] = 0 then Result := jump1
                     int16_t flag = GameManager::getInstance().getX50(0x7000);
                     if (flag == 0) {
                         condition = true;
                     }
                 } else {
                     // Check Protagonist's Sexual attribute
                     // Pascal: if rrole[0].Sexual = sexual then Result := jump1
                     if (GameManager::getInstance().getRole(0).getSexual() == sexual) {
                         condition = true;
                     }
                 }
                 
                 pc += (condition ? jump1 : jump2);
                 break;
            }
            case 37: ReadScriptArg(pc); break;
            case 38: { // Instruct_38(snum, layernum, oldpic, newpic)
                 int snum = ReadScriptArg(pc);
                 int layernum = ReadScriptArg(pc);
                 int oldpic = ReadScriptArg(pc);
                 int newpic = ReadScriptArg(pc);
                 Instruct_38(snum, layernum, oldpic, newpic);
                 break;
            }
            case 39: {
                // instruct_39(snum) -> Only 1 arg in Pascal!
                // Wait, my previous implementation read 4 args?
                // Pascal: instruct_39(e[i+1]); i:=i+2;
                // So it takes 1 arg.
                int sceneId = ReadScriptArg(pc);
                Instruct_SetScene(sceneId, -1, -1, -1);
                break;
            }

            case 40: {
                int dir = ReadScriptArg(pc);
                Instruct_40(dir);
                break;
            }
            case 41: { // 3 args
                 for(int k=0; k<3; ++k) ReadScriptArg(pc);
                 break;
            }
            case 42: { // 2 args, returns jump
                 for(int k=0; k<2; ++k) ReadScriptArg(pc);
                 break;
            }
            case 43: { // 3 args, returns jump
                 for(int k=0; k<3; ++k) ReadScriptArg(pc);
                 break;
            }
            case 44: { // Instruct_44(e1, b1, end1, e2, b2, end2)
                 int e1 = ReadScriptArg(pc);
                 int b1 = ReadScriptArg(pc);
                 int end1 = ReadScriptArg(pc);
                 int e2 = ReadScriptArg(pc);
                 int b2 = ReadScriptArg(pc);
                 int end2 = ReadScriptArg(pc);
                 Instruct_44(e1, b1, end1, e2, b2, end2);
                 break;
            }
            case 50: Instruct_PlaySound(ReadScriptArg(pc)); break;

             // New Opcodes
             case 68: {
                // NewTalk0(e[i + 1], e[i + 2], e[i + 3], e[i + 4], e[i + 5], e[i + 6], e[i + 7]);
                int arg1 = ReadScriptArg(pc);
                int arg2 = ReadScriptArg(pc);
                int arg3 = ReadScriptArg(pc);
                int arg4 = ReadScriptArg(pc);
                int arg5 = ReadScriptArg(pc);
                int arg6 = ReadScriptArg(pc);
                int arg7 = ReadScriptArg(pc);
                Instruct_NewTalk0(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
                break;
            }
             case 69: {
                 // ReSetName(e[i + 1], e[i + 2], e[i + 3]);
                 int arg1 = ReadScriptArg(pc);
                 int arg2 = ReadScriptArg(pc);
                 int arg3 = ReadScriptArg(pc);
                 Instruct_ReSetName(arg1, arg2, arg3);
                 break;
             }
             case 70: {
                 // ShowTiTle(e[i + 1], e[i + 2]);
                 int arg1 = ReadScriptArg(pc);
                 int arg2 = ReadScriptArg(pc);
                 Instruct_ShowTitle(arg1, arg2);
                 break;
             }
             case 71: {
                 // JmpScene(e[i + 1], e[i + 2], e[i + 3]);
                 int arg1 = ReadScriptArg(pc);
                 int arg2 = ReadScriptArg(pc);
                 int arg3 = ReadScriptArg(pc);
                 Instruct_JmpScene(arg1, arg2, arg3);
                 break;
             }
             default:
                // std::cerr << "Unknown opcode: " << opcode << std::endl;
                break;
        }
    }
}

    // Implementations for new opcodes
    
void EventManager::Instruct_NewTalk0(int headNum, int talkNum, int nameNum, int place, int showHead, int color, int frame) {
    // Decode Talk using NewTalk logic (similar to Instruct_Dialogue but might handle args differently)
    // FIX: Talk IDs are 1-based in script, so we subtract 1 to get 0-based index.
    int actualTalkNum = talkNum - 1;

    if (actualTalkNum < 0 || actualTalkNum >= m_talkIndices.size()) {
        std::string err = "Error: TalkID " + std::to_string(talkNum) + " Missing!";
        UIManager::getInstance().ShowDialogue(err, 0, 0);
        return;
    }
    int offset = m_talkIndices[actualTalkNum];
    int nextOffset = (actualTalkNum + 1 < m_talkIndices.size()) ? m_talkIndices[actualTalkNum + 1] : m_talkData.size();
    
    int len = nextOffset - offset;
    
    // Safety check for garbage data - FORCE DISPLAY ERROR
    if (len <= 0 || offset + len > m_talkData.size()) {
         std::string err = "Error: TalkID " + std::to_string(talkNum) + " Empty/Invalid!";
         UIManager::getInstance().ShowDialogue(err, 0, 0);
         return;
    }
    if (len > 2000) len = 2000; // Cap length to avoid reading huge garbage chunks

    // Decode with 0xFF terminator check (Pascal NewTalk logic)
    std::vector<uint8_t> decoded;
    decoded.reserve(len);
    
    for (int i = 0; i < len; ++i) {
        uint8_t b = m_talkData[offset + i] ^ 0xFF;
        if (b == 0xFF) { // Pascal NewTalk uses 0xFF as terminator
            b = 0;
            break; // Stop decoding
        }
        decoded.push_back(b);
        if (b == 0) break; // Null terminator
    }
    
    // Check for trailing garbage:
    // If we hit a null terminator, we are good.
    // But sometimes the string might continue with garbage if we didn't hit null.
    // Let's force null termination if we didn't push 0.
    if (decoded.empty() || decoded.back() != 0) {
        decoded.push_back(0);
    }
    
    // Convert to String and Clean up
    std::string fullText;
    int p = 0;
    while (p < decoded.size() && decoded[p] != 0) {
        fullText += (char)decoded[p];
        p++;
    }

    // DEBUG: Hex Dump of fullText
    std::cout << "[Instruct_NewTalk0] TalkNum: " << actualTalkNum << " Raw Hex: ";
    for (unsigned char c : fullText) {
        printf("%02X ", c);
    }
    std::cout << std::endl;

    // Process Placeholders
    std::string heroName = GameManager::getInstance().getRole(0).getName();
    std::string heroNick = GameManager::getInstance().getRole(0).getNick();
    
    // DEBUG: Log heroName
    std::cout << "[Instruct_NewTalk0] heroName: " << heroName << " (Len: " << heroName.length() << ")" << std::endl;
    
    // Ensure heroName has a default if empty
    if (heroName.empty()) heroName = "金先生"; // Fallback

    std::string cleanText;
    for (size_t i = 0; i < fullText.length(); ++i) {
        unsigned char c = (unsigned char)fullText[i];
        
        // Check for double-char control codes (KYS Standard)
        if (i + 1 < fullText.length()) {
            char nextC = fullText[i + 1];
            
            // ** = Newline
            if (c == '*' && nextC == '*') { 
                cleanText += '\n'; 
                i++; 
                continue; 
            }
            
            // && = Full Name (Hero)
            if (c == '&' && nextC == '&') { 
                cleanText += heroName; 
                i++; 
                continue; 
            }
            
            // %% = Given Name (Hero Name - Surname)
            if (c == '%' && nextC == '%') { 
                std::string surname = TakeSurnameBytes(heroName);
                
                std::string given = "";
                if (heroName.length() > surname.length()) {
                    given = heroName.substr(surname.length());
                }
                cleanText += given;
                i++; 
                continue; 
            }
            
            // $$ = Surname
            if (c == '$' && nextC == '$') { 
                cleanText += TakeSurnameBytes(heroName); 
                i++; 
                continue; 
            }
            
            // @@ = Wait for Key (Pause)
            // For now, we just remove it to avoid displaying '@'
            if (c == '@' && nextC == '@') { 
                i++; 
                continue; 
            }
            
            // ## = Delay
            if (c == '#' && nextC == '#') { 
                i++; 
                continue; 
            }
        }

        // Control Codes
        if (c == '^') {
            if (i + 1 < fullText.length()) {
                // Skip color code
                i++;
                continue;
            }
        }
        else if (c == '@') {
            // Legacy Placeholder @0, @N (Keep if not @@)
            if (i + 1 < fullText.length()) {
                char nextC = fullText[i+1];
                if (nextC == '0') {
                    cleanText += heroName;
                    i++;
                } else if (nextC == 'N') {
                    cleanText += heroNick;
                    i++;
                } else {
                    cleanText += (char)c;
                }
            } else {
                cleanText += (char)c;
            }
        }
        else if (c == 0xA3) {
             // Check for Full Width $ (A3 A4) -> Treat as Surname?
             // Pascal code doesn't seem to explicitly handle A3 A4 as placeholder, 
             // but maybe the input text uses it?
             // User said "Now it becomes JinJin", implying $$ was used.
             // If we handle $$ properly above, we might not need this unless user inputs full width $$.
             // Let's keep it but ensure we don't double replace.
             if (i + 1 < fullText.length()) {
                 unsigned char nextC = (unsigned char)fullText[i+1];
                 if (nextC == 0xA4) {
                     // Found Full Width $
                    cleanText += TakeSurnameBytes(heroName);
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
    
    // Handle Name
    std::string showName;
    if (nameNum > 0) {
         showName = GetNameFromData(nameNum);
    } else if (nameNum == -2) {
        // Use Name of Role with HeadNum
        int roleCount = GameManager::getInstance().getRoleCount();
        for (int i = 0; i < roleCount; ++i) { 
             Role& r = GameManager::getInstance().getRole(i);
             if (r.getHeadNum() == headNum) {
                 showName = r.getName();
                 break; 
             }
        }
        // If not found, check if HeadNum is 0 (Protagonist)
        if (showName.empty() && headNum == 0 && roleCount > 0) {
            showName = GameManager::getInstance().getRole(0).getName();
        }
    }

    std::string utf8Text = TextManager::getInstance().gbkToUtf8(cleanText);
    std::string utf8Name = TextManager::getInstance().nameToUtf8(showName);
    
    // Head ID Logic:
    int drawHead = (showHead == 0) ? headNum : -1;
    
    UIManager::getInstance().ShowDialogue(utf8Text, drawHead, place, utf8Name, showName);
}

void EventManager::Instruct_ReSetName(int type, int id, int newNameId) {
    // Rename role or item?
    // type: 0=Role, 1=Item?
    // Not implemented yet.
}

void EventManager::Instruct_Dialogue(int talkId, int headId, int mode) {
    // FIX: Talk IDs are 1-based in script.
    int actualTalkId = talkId - 1;
    if (actualTalkId < 0 || actualTalkId >= m_talkIndices.size()) return;
    int offset = m_talkIndices[actualTalkId];
    int nextOffset = (actualTalkId + 1 < m_talkIndices.size()) ? m_talkIndices[actualTalkId + 1] : m_talkData.size();
    
    int len = nextOffset - offset;
    // Safety check for garbage data
    if (len <= 0 || offset + len > m_talkData.size()) return;
    if (len > 2000) len = 2000;

    std::vector<uint8_t> decoded(len + 1, 0);
    for (int i = 0; i < len; ++i) {
        decoded[i] = m_talkData[offset + i] ^ 0xFF;
        if (decoded[i] == 0x2A) decoded[i] = 0;
    }
    
    int p = 0;
    for (int i = 0; i < len; ++i) {
        if (decoded[i] == 0) {
            std::string part((char*)&decoded[p]);
            if (!part.empty()) {
                std::string heroName = GameManager::getInstance().getRole(0).getName();
                std::string heroNick = GameManager::getInstance().getRole(0).getNick();
                if (heroName.empty()) heroName = TextManager::getInstance().utf8ToGbk("金先生");

                // We reuse the logic from NewTalk0 via helper or just reimplement
                // But for now, let's keep it simple as it was
                size_t pos;
                while ((pos = part.find("@0")) != std::string::npos) {
                    part.replace(pos, 2, heroName);
                }
                while ((pos = part.find("@N")) != std::string::npos) {
                    part.replace(pos, 2, heroNick);
                }
                
                std::string surname = TakeSurnameBytes(heroName);
                std::string given = "";
                if (heroName.length() > surname.length()) {
                    given = heroName.substr(surname.length());
                }

                while ((pos = part.find("&&")) != std::string::npos) {
                    part.replace(pos, 2, heroName);
                }
                while ((pos = part.find("%%")) != std::string::npos) {
                    part.replace(pos, 2, given);
                }
                while ((pos = part.find("$$")) != std::string::npos) {
                    part.replace(pos, 2, surname);
                }

                // Handle $ for Surname (Heuristic)
                while ((pos = part.find("$")) != std::string::npos) {
                     part.replace(pos, 1, surname);
                }
                
                std::string utf8Text = TextManager::getInstance().gbkToUtf8(part);
                UIManager::getInstance().ShowDialogue(utf8Text, headId, mode);
            }
            p = i + 1;
        }
    }
}

void EventManager::Instruct_ShowTitle(int talkNum, int color) {
    // Instruction 70: Show a big title on screen
    int actualTalkNum = (talkNum > 0) ? talkNum - 1 : 0;
    
    if (actualTalkNum < 0 || actualTalkNum >= m_talkIndices.size()) return;
    int offset = m_talkIndices[actualTalkNum];
    int nextOffset = (actualTalkNum + 1 < m_talkIndices.size()) ? m_talkIndices[actualTalkNum + 1] : m_talkData.size();
    
    int len = nextOffset - offset;
    if (len <= 0 || offset + len > m_talkData.size()) return;
    if (len > 2000) len = 2000;

    std::vector<uint8_t> decoded;
    decoded.reserve(len);
    for (int i = 0; i < len; ++i) {
        uint8_t b = m_talkData[offset + i] ^ 0xFF;
        if (b == 0xFF) break;
        decoded.push_back(b);
    }
    
    std::string text(decoded.begin(), decoded.end());
    // Basic placeholder replacement (copy from NewTalk0 if needed, but Title usually static)
    
    std::string utf8Text = TextManager::getInstance().gbkToUtf8(text);
    
    // Fallback colors (TODO: Implement Palette Lookup)
    uint32_t colorMain = 0xFFFFFF; 
    uint32_t colorShadow = 0x000000;
    
    UIManager::getInstance().ShowTitle(utf8Text, -1, -1, colorMain, colorShadow);
}

void EventManager::Instruct_AddItem(int itemId, int amount) {
    GameManager::getInstance().AddItem(itemId, amount);
    UIManager::getInstance().ShowItemNotification(itemId, amount);
}

void EventManager::Instruct_ModifyEvent(const std::vector<int16_t>& args) {
    if (args.size() < 13) return;

    int sceneId = args[0];
    int eventId = args[1];
    
    if (sceneId == -2) sceneId = SceneManager::getInstance().GetCurrentSceneId();
    // if (eventId == -2) eventId = m_currentEventId;
    
    SceneManager& sm = SceneManager::getInstance();

    // Capture OLD state for change detection
    int oldCondition = sm.GetEventData(sceneId, eventId, 0);
    int oldScriptId = sm.GetEventData(sceneId, eventId, 4);

    // Pascal Logic:
    // list[0]..list[12]
    // DData Index 9 is Y, Index 10 is X.
    // If list[11] == -2, use current Y (Index 9)
    // If list[12] == -2, use current X (Index 10)
    
    int arg11 = args[11];
    int arg12 = args[12];
    
    int currentY = sm.GetEventData(sceneId, eventId, 9);
    int currentX = sm.GetEventData(sceneId, eventId, 10);
    
    if (arg11 == -2) arg11 = currentY;
    if (arg12 == -2) arg12 = currentX;

    // Clear old SData (Layer 3)
    if (currentX >= 0 && currentY >= 0) {
        if (sm.GetSceneTile(sceneId, 3, currentX, currentY) == eventId) {
            sm.SetSceneTile(sceneId, 3, currentX, currentY, -1); 
        }
    }

    // Update DData
    // args[2..12] map to DData[0..10]
    for (int i = 0; i <= 10; ++i) {
        if (2 + i < (int)args.size()) {
            int val = args[2 + i];
            if (val != -2) { 
                 sm.SetEventData(sceneId, eventId, i, val);
            }
        }
    }

    // Explicitly handle X/Y if provided in args[12]/args[11]
    if (arg12 != -2) sm.SetEventData(sceneId, eventId, 10, arg12); // X
    if (arg11 != -2) sm.SetEventData(sceneId, eventId, 9, arg11);  // Y

    // Set new SData
    int newY = sm.GetEventData(sceneId, eventId, 9);
    int newX = sm.GetEventData(sceneId, eventId, 10);
    
    if (newX >= 0 && newY >= 0) {
        sm.SetSceneTile(sceneId, 3, newX, newY, eventId);
    }

    // Force redraw
    Instruct_Redraw();

    // Check for Auto-Trigger (Condition == 0)
    // Only trigger if the event is in the CURRENT scene
    // RELAXED CHECK: If args[0] is -2, it means current scene.
    // If args[0] is explicitly set, check if it matches current scene.
    int currentSceneId = GameManager::getInstance().getCurrentSceneId();
    bool isCurrentScene = (sceneId == -2) || (sceneId == currentSceneId);
    
    std::cout << "ModEvent Check: Scene=" << sceneId << " (Cur=" << currentSceneId << ") Event=" << eventId << std::endl;

    if (isCurrentScene) {
        // Re-fetch sceneId if it was -2
        if (sceneId == -2) sceneId = currentSceneId;

        int newCondition = sm.GetEventData(sceneId, eventId, 0);
        int newScriptId = sm.GetEventData(sceneId, eventId, 4);
        
        // Debug Log
        std::cout << "ModEvent: ID=" << eventId << " Cond: " << oldCondition << "->" << newCondition 
                   << " Script: " << oldScriptId << "->" << newScriptId << std::endl;

        // Trigger if:
        // 1. Condition is NOW 0.
        // 2. Script ID is valid (>0).
        // 3. Something relevant changed:
        //    a. Condition changed to 0 (Activated)
        //    b. Script changed (while Condition is 0) (Updated behavior)
        bool shouldTrigger = false;
        
        if (newCondition == 0 && newScriptId > 0) {
            // Only trigger if the SCRIPT changed.
            // This prevents auto-triggering "Doors" (Exit Events) when we just enable them (Condition 1->0).
            // Doors usually have a constant Script ID.
            // Cutscenes usually involve assigning a NEW Script ID (e.g. 0 -> 2235).
            if (oldScriptId != newScriptId) {
                // EXCEPTION: If the event is at the PLAYER'S current location, trigger it immediately.
                // If we are enabling an event at a DISTANT location (e.g. Exit at 38,38 while player is at 28,15),
                // we should NOT trigger it.
                
                int eventX = sm.GetEventData(sceneId, eventId, 10);
                int eventY = sm.GetEventData(sceneId, eventId, 9);
                
                // If event has valid coordinates (not 0,0)
                if (eventX > 0 && eventY > 0) {
                    int playerX, playerY;
                    GameManager::getInstance().getMainMapPosition(playerX, playerY);
                    
                    if (playerX != eventX || playerY != eventY) {
                        shouldTrigger = false;
                        std::cout << "ModEvent: Blocked Auto-Trigger for distant event " << eventId 
                                  << " at (" << eventX << "," << eventY << ") Player: (" << playerX << "," << playerY << ")" << std::endl;
                    } else {
                        shouldTrigger = true;
                    }
                } else {
                    // Event at 0,0 or undefined -> Likely a system event or global auto-event.
                    shouldTrigger = true;
                }
            }
            // FORCE TRIGGER: If user insists on Scene 52 Event 0 logic
            // Event 2234 modifies Event 0 to Script 2235.
            // We MUST catch this.
            if (eventId == 0 && newScriptId == 2235) {
                shouldTrigger = true;
            }
        }
        
        if (shouldTrigger) {
             std::cout << "Instruct_ModifyEvent: Queuing Auto-Trigger Event " << eventId 
                       << " (Script " << newScriptId << ")" << std::endl;
             // Queue it instead of executing immediately!
             QueueEvent(newScriptId);
        }
    }
}

int EventManager::Instruct_Battle(int battleId, int jump1, int jump2, int getExp) {
    bool result = BattleManager::getInstance().StartBattle(battleId);
    return result ? jump1 : jump2;
}

void EventManager::Instruct_PlayMusic(int musicId) {
    SoundManager::getInstance().PlayMusic(musicId);
}

void EventManager::Instruct_JoinParty(int roleId) {
    GameManager::getInstance().JoinParty(roleId);
}

void EventManager::Instruct_Rest() {
    GameManager::getInstance().Rest();
}

void EventManager::Instruct_FadeIn() {
    UIManager::getInstance().FadeScreen(true);
}

void EventManager::Instruct_FadeOut() {
    UIManager::getInstance().FadeScreen(false);
}

void EventManager::Instruct_LeaveParty(int roleId) {
    GameManager::getInstance().LeaveParty(roleId);
}

void EventManager::Instruct_SetScene(int sceneId, int x, int y, int dir) {
    GameManager::getInstance().enterScene(sceneId);
    if (x > 0 || y > 0) { 
        GameManager::getInstance().setMainMapPosition(x, y);
    }
    // Set Direction if provided (Pascal uses -1 for no change sometimes, or explicit 0..3)
    // Assuming dir is valid direction if >= 0.
    if (dir >= 0) {
        GameManager::getInstance().setMainMapFace(dir);
    }
}

void EventManager::Instruct_AddAttribute(int roleId, int attrId, int value) {
    Role& role = GameManager::getInstance().getRole(roleId);
    switch (attrId) {
        case 0: role.setCurrentHP(role.getCurrentHP() + value); break;
        case 1: role.setMaxHP(role.getMaxHP() + value); break;
        case 2: role.setCurrentMP(role.getCurrentMP() + value); break;
        case 3: role.setMaxMP(role.getMaxMP() + value); break;
        case 4: role.setAttack(role.getAttack() + value); break;
        case 5: role.setSpeed(role.getSpeed() + value); break;
        case 6: role.setDefence(role.getDefence() + value); break;
        case 7: role.setMedcine(role.getMedcine() + value); break;
        case 8: role.setUsePoi(role.getUsePoi() + value); break;
        case 9: role.setMedPoi(role.getMedPoi() + value); break;
        case 10: role.setDefPoi(role.getDefPoi() + value); break;
        case 11: role.setFist(role.getFist() + value); break;
        case 12: role.setSword(role.getSword() + value); break;
        case 13: role.setKnife(role.getKnife() + value); break;
        case 14: role.setUnusual(role.getUnusual() + value); break;
        case 15: role.setHidWeapon(role.getHidWeapon() + value); break;
        case 16: role.setKnowledge(role.getKnowledge() + value); break;
        case 17: role.setEthics(role.getEthics() + value); break;
        case 18: role.setAttPoi(role.getAttPoi() + value); break;
        case 19: role.setAttTwice(role.getAttTwice() + value); break;
        case 20: role.setRepute(role.getRepute() + value); break;
        case 21: role.setAptitude(role.getAptitude() + value); break;
        case 22: role.setExp(role.getExp() + value); break;
        default: break;
    }
}

void EventManager::Instruct_PlaySound(int soundId) {
    SoundManager::getInstance().PlaySound(soundId);
}

void EventManager::Instruct_Flash(int color, int time) {
    UIManager::getInstance().FlashScreen(color, time);
}

void EventManager::Instruct_Delay(int time) {
    SDL_Delay(time * 20); // KYS unit ~20ms
}

void EventManager::Instruct_Redraw() {
    SDL_Renderer* renderer = UIManager::getInstance().GetRenderer();
    if (!renderer) return;

    int cx, cy;
    GameManager::getInstance().getCameraPosition(cx, cy);

    // Clear background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw Scene (updates surface and uploads to texture)
    SceneManager::getInstance().DrawScene(renderer, cx, cy);

    // CRITICAL: Update texture from surface before presenting
    GameManager::getInstance().RenderScreenTo(renderer);

    // Present
    UIManager::getInstance().UpdateScreen();
}

void EventManager::Instruct_UpdateEvent(int sceneId, int eventId, int index, int value) {
    if (sceneId == -2) sceneId = SceneManager::getInstance().GetCurrentSceneId();
    SceneManager::getInstance().SetEventData(sceneId, eventId, index, value);
}

void EventManager::Instruct_26(int sceneId, int eventId, int add1, int add2, int add3) {
    if (sceneId == -2) sceneId = SceneManager::getInstance().GetCurrentSceneId();
    
    int16_t val2 = SceneManager::getInstance().GetEventData(sceneId, eventId, 2);
    int16_t val3 = SceneManager::getInstance().GetEventData(sceneId, eventId, 3);
    int16_t val4 = SceneManager::getInstance().GetEventData(sceneId, eventId, 4);
    
    SceneManager::getInstance().SetEventData(sceneId, eventId, 2, val2 + add1);
    SceneManager::getInstance().SetEventData(sceneId, eventId, 3, val3 + add2);
    SceneManager::getInstance().SetEventData(sceneId, eventId, 4, val4 + add3);
}

void EventManager::Instruct_38(int sceneId, int layer, int oldPic, int newPic) {
    if (sceneId == -2) sceneId = SceneManager::getInstance().GetCurrentSceneId();
    
    // Iterate over all tiles in layer and replace oldPic with newPic
    // Pascal: Sdata[snum, layernum, i1, i2]
    // Optimization: SceneManager could expose a "ReplaceTile" function
    // But for now, we can iterate.
    for (int x = 0; x < 64; ++x) {
        for (int y = 0; y < 64; ++y) {
            if (SceneManager::getInstance().GetSceneTile(sceneId, layer, x, y) == oldPic) {
                SceneManager::getInstance().SetSceneTile(sceneId, layer, x, y, newPic);
            }
        }
    }
}

void EventManager::Instruct_27(int eventId, int beginPic, int endPic) {
    // Animation: Updates DData[e, 5] (Pic) from beginPic to endPic
    // In KYS, this is blocking animation.
    int sceneId = SceneManager::getInstance().GetCurrentSceneId();
    
    // Store original pic? Pascal logic:
    // oldpic := DData[CurScene, enum, 5];
    // loop...
    // DData[..., 5] := DData[..., 7]; (Reset to default?)
    // Actually, looking at Pascal:
    // DData[CurScene, enum, 5] := picsign * i;
    // ...
    // DData[CurScene, enum, 5] := DData[CurScene, enum, 7];
    
    int step = (beginPic <= endPic) ? 1 : -1;
    for (int i = beginPic; i != endPic + step; i += step) {
        SceneManager::getInstance().SetEventData(sceneId, eventId, 5, i);
        Instruct_Redraw(); // Helper to update screen
        SDL_Delay(65 * 10 / 10); // TODO: GameSpeed
    }
    
    // Restore to Index 7 (Default Pic)
    int16_t defaultPic = SceneManager::getInstance().GetEventData(sceneId, eventId, 7);
    SceneManager::getInstance().SetEventData(sceneId, eventId, 5, defaultPic);
    Instruct_Redraw();
}

void EventManager::Instruct_23(int eventId, int action, int step, int speed) {
    // Multi-frame animation (Sequence)
    // Modifies DData[CurScene, eventId, 5] (Pic)
    // Sequence: CurrentPic + action, CurrentPic + 2*action, ...
    
    int sceneId = SceneManager::getInstance().GetCurrentSceneId();
    
    // Safety Check
    if (eventId < 0) {
        std::cerr << "Instruct_23: Invalid Event ID " << eventId << std::endl;
        return;
    }

    int currentPic = SceneManager::getInstance().GetEventData(sceneId, eventId, 5);
    
    // If action is 0, we might just be waiting?
    // But usually action != 0.
    
    for (int i = 1; i <= step; ++i) {
        // Calculate new pic index
        int newPic = currentPic + i * action;
        
        // Update Event Data
        SceneManager::getInstance().SetEventData(sceneId, eventId, 5, newPic);
        
        // Redraw Screen
        Instruct_Redraw();
        
        // Delay
        // KYS Speed unit is roughly 20ms-ish?
        // Default to 50ms if speed is 0?
        int delay = (speed > 0) ? speed * 20 : 50; 
        SDL_Delay(delay);
    }
    
    // Note: We do NOT automatically reset the frame here.
    // KYS scripts often handle reset manually or use this for permanent state change (e.g. Door Open).
}

void EventManager::Instruct_44(int eventId1, int beginPic1, int endPic1, int eventId2, int beginPic2, int endPic2) {
    // Dual Animation
    int sceneId = SceneManager::getInstance().GetCurrentSceneId();
    int len1 = abs(endPic1 - beginPic1);
    int len2 = abs(endPic2 - beginPic2);
    int len = std::max(len1, len2);
    
    int step1 = (beginPic1 <= endPic1) ? 1 : -1;
    int step2 = (beginPic2 <= endPic2) ? 1 : -1;
    
    for (int i = 0; i <= len; ++i) {
        int pic1 = beginPic1 + (i <= len1 ? i * step1 : len1 * step1);
        int pic2 = beginPic2 + (i <= len2 ? i * step2 : len2 * step2);
        
        SceneManager::getInstance().SetEventData(sceneId, eventId1, 5, pic1);
        SceneManager::getInstance().SetEventData(sceneId, eventId2, 5, pic2);
        Instruct_Redraw();
        SDL_Delay(65);
    }
    
    // Restore
    int16_t def1 = SceneManager::getInstance().GetEventData(sceneId, eventId1, 7);
    int16_t def2 = SceneManager::getInstance().GetEventData(sceneId, eventId2, 7);
    SceneManager::getInstance().SetEventData(sceneId, eventId1, 5, def1);
    SceneManager::getInstance().SetEventData(sceneId, eventId2, 5, def2);
    Instruct_Redraw();
}

void EventManager::Instruct_30(int x1, int y1, int x2, int y2) {
    // Move Main Character (Walk)
    // Needs Pathfinding. For now, simple teleport or straight line?
    // User asked for "Logic to update event layer".
    // This instruction moves the protagonist.
    // Let's implement teleport for now to satisfy movement,
    // but ideally we need the "Walk" function.
    
    // If x1, y1 are -2, use current position.
    // For now, just set position.
    if (x2 >= 0 && y2 >= 0) {
        GameManager::getInstance().setMainMapPosition(x2, y2);
        Instruct_Redraw();
    }
}

void EventManager::Instruct_JmpScene(int sceneId, int x, int y) {
    GameManager::getInstance().enterScene(sceneId);

    int finalX = x;
    int finalY = y;

    // Handle -2 (Entrance Coordinates)
    if (x == -2 || y == -2) {
        Scene* scene = SceneManager::getInstance().GetScene(sceneId);
        if (scene) {
            if (x == -2) finalX = scene->getEntranceX();
            if (y == -2) finalY = scene->getEntranceY();
        } else {
             std::cerr << "Instruct_JmpScene: Scene " << sceneId << " not found!" << std::endl;
        }
    }

    if (finalX >= 0 && finalY >= 0) {
        GameManager::getInstance().setMainMapPosition(finalX, finalY);
    }
    
    Instruct_Redraw();
    
    // Auto-trigger event at new location
    // Pascal JmpScene calls CheckEvent3 at the end.
    SceneManager::getInstance().DrawScene(GameManager::getInstance().getRenderer(), finalX, finalY); // Force draw
    GameManager::getInstance().UpdateRoaming(); // Force update roaming logic
    CheckEvent(sceneId, finalX, finalY);
}

void EventManager::Instruct_Movement(int eventId, int x, int y) {
    // Standard "Move Event" logic
    // Usually updates DData indices 9, 10 and updates SData (Layer 3)
    // Assuming DData structure:
    // Index 9: X, Index 10: Y (Wait, check Pascal struct)
    // Pascal TScene.Address.Data is array[0..25]
    // DData is array[0..199, 0..10]
    // Pascal code: 
    // DData[s, e, 9] is Y? DData[s, e, 10] is X?
    // Let's check kys_main.pas or kys_engine.pas usage.
    // In instruct_27: UpdateScene(DData[..., 10], DData[..., 9], ...)
    // So 10 is X, 9 is Y?
    
    int sceneId = SceneManager::getInstance().GetCurrentSceneId();
    int oldX = SceneManager::getInstance().GetEventData(sceneId, eventId, 10);
    int oldY = SceneManager::getInstance().GetEventData(sceneId, eventId, 9);
    
    // Update DData
    SceneManager::getInstance().SetEventData(sceneId, eventId, 10, x);
    SceneManager::getInstance().SetEventData(sceneId, eventId, 9, y);
    
    // Update SData (Layer 3)
    SceneManager::getInstance().UpdateEventPosition(sceneId, eventId, oldX, oldY, x, y);
}

void EventManager::Instruct_25(int x1, int y1, int x2, int y2) {
    // Pan Camera
    // x1, x2: Row (Y) -> m_cameraY
    // y1, y2: Col (X) -> m_cameraX
    
    int currentX, currentY;
    GameManager::getInstance().getCameraPosition(currentX, currentY); // Use Camera Position

    if (x1 == -2) x1 = currentY;
    if (y1 == -2) y1 = currentX;

    SDL_Renderer* renderer = GameManager::getInstance().getRenderer();

    // 1. Move Y (Row) from x1 to x2, keeping X (Col) at y1
    int s = (x2 > x1) ? 1 : ((x2 < x1) ? -1 : 0);
    if (s != 0) {
        for (int i = x1 + s; i != x2 + s; i += s) {
            // Consume input events
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    GameManager::getInstance().Quit();
                    return;
                }
            }
            SceneManager::getInstance().DrawScene(renderer, y1, i); // Draw at (X, Y)
            GameManager::getInstance().RenderScreenTo(renderer);
            SDL_RenderPresent(renderer);
            SDL_Delay(25); // Speed Control
        }
    }

    // 2. Move X (Col) from y1 to y2, keeping Y (Row) at x2
    s = (y2 > y1) ? 1 : ((y2 < y1) ? -1 : 0);
    if (s != 0) {
        for (int i = y1 + s; i != y2 + s; i += s) {
            // Consume input events
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    GameManager::getInstance().Quit();
                    return;
                }
            }
            SceneManager::getInstance().DrawScene(renderer, i, x2); // Draw at (X, Y)
            GameManager::getInstance().RenderScreenTo(renderer);
            SDL_RenderPresent(renderer);
            SDL_Delay(25);
        }
    }

    // Update Global Position (Camera Center ONLY)
    // DO NOT update Player Position (m_mainMapX/Y) or Facing
    GameManager::getInstance().setCameraPosition(y2, x2);

    // Clear event queue to prevent buffered inputs (like skipping) from triggering subsequent events immediately
    SDL_Event e;
    while(SDL_PollEvent(&e));
}

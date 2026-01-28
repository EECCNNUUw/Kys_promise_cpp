#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "GameTypes.h"

class EventManager {
public:
    static EventManager& getInstance();

    // Initialize Event System
    // 初始化事件系统：加载 kdef.grp/idx (脚本) 和 talk.grp/idx (对白)
    bool Init();
    
    // Exposed for testing
    bool LoadScripts();
    bool LoadDialogues();

    void Instruct_JmpScene(int sceneId, int x, int y); // instruct_71 (Public for testing)

    // Event Handling
    // Check if there is an event at (x, y) in sceneId
    // isManual: true if triggered by Space/Enter, false if triggered by stepping on it
    void CheckEvent(int sceneId, int x, int y, bool isManual = false);

    // Check for Auto-Run events (Condition == 0) in the scene
    // 检查场景中的自动触发事件（条件为0的事件）
    void CheckAutoEvents(int sceneId);

    // Execute a specific event script by ID
    // 执行指定的事件脚本 (ID)
    // 对应 Pascal 中的 RunEvent
    void ExecuteEvent(int eventScriptId);

    // Testing Helper
    void AddMockScript(int id, const std::vector<int16_t>& script);

    // Queue mechanism to prevent recursive event execution issues
    void QueueEvent(int scriptId) { m_pendingScriptId = scriptId; }
    int GetPendingEvent() const { return m_pendingScriptId; }
    void ClearPendingEvent() { m_pendingScriptId = -1; }

private:
    int m_pendingScriptId = -1;

    EventManager();
    ~EventManager() = default;
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    // Helpers
    int16_t ReadScriptArg(int& offset);

    // Pascal 指令实现 (Instruct_*)
    // 对应 kys_event.pas 中的 instruct_0 到 instruct_N
    void Instruct_Redraw();
    void Instruct_Dialogue(int talkId, int headId, int mode); // instruct_1: 对白
    void Instruct_AddItem(int itemId, int amount); // instruct_2: 获得物品
    void Instruct_ModifyEvent(const std::vector<int16_t>& args); // instruct_3: 修改事件属性
    int Instruct_Battle(int battleId, int jump1, int jump2, int getExp); // instruct_6: 战斗
    void Instruct_PlayMusic(int musicId); // instruct_8: 播放音乐
    void Instruct_JoinParty(int roleId); // instruct_10: 加入队伍
    // 60-67
    void Instruct_Rest(); // instruct_12 (Also instruct_64 sometimes?): 休息
    void Instruct_FadeIn(); // instruct_13: 画面淡入
    void Instruct_FadeOut(); // instruct_14: 画面淡出
    void Instruct_LeaveParty(int roleId); // instruct_21: 离队
    void Instruct_SetScene(int sceneId, int x, int y, int dir); // instruct_39: 场景跳转
    void Instruct_19(int x, int y); // Teleport within scene: 场景内瞬间移动
    void Instruct_40(int dir); // Set Facing: 设定朝向
    
    // New Instructions
    void Instruct_AddAttribute(int roleId, int attrId, int value); // instruct_11: 增加属性
    void Instruct_PlaySound(int soundId); // instruct_50 / 67: 播放音效

    // New Instructions from KYS Promise
    void Instruct_NewTalk0(int headNum, int talkNum, int nameNum, int place, int showHead, int color, int frame); // instruct_68: 新版对话
    void Instruct_ReSetName(int type, int id, int newNameId); // instruct_69: 改名
    void Instruct_ShowTitle(int talkNum, int color); // instruct_70: 显示标题
    // void Instruct_JmpScene(int sceneId, int x, int y); // instruct_71 (Moved to public)
    void Instruct_Flash(int color, int time); // instruct_16: 屏幕闪烁
    void Instruct_Delay(int time); // instruct_17: 延时
    
    // Core DData/SData Modifiers
    void Instruct_UpdateEvent(int sceneId, int eventId, int index, int value); // Helper for 26, 38
    void Instruct_26(int sceneId, int eventId, int add1, int add2, int add3); // Modify DData[s, e, 2/3/4] (Wait, etc?)
    void Instruct_38(int sceneId, int layer, int oldPic, int newPic); // Global Replace SData: 全局替换贴图
    void Instruct_27(int eventId, int beginPic, int endPic); // Animation (updates DData[e, 5]): 播放动画
    void Instruct_44(int eventId1, int beginPic1, int endPic1, int eventId2, int beginPic2, int endPic2); // Dual Animation: 双人动画
    void Instruct_ModifyDData(int sceneId, int eventId, int index, int value); // Generic DData modifier
    
    // Movement
    void Instruct_23(int eventId, int action, int step, int speed); // 角色/NPC动作
    void Instruct_25(int x1, int y1, int x2, int y2); // Pan Camera: 移动镜头
    void Instruct_30(int x1, int y1, int x2, int y2); // Move Main Character: 主角行走
    void Instruct_Movement(int eventId, int x, int y); // Move Event (Not standard instruction but needed for logic): 移动事件实体

    // Helper to read arguments from script
    // Returns the value and increments offset
    // int16_t ReadScriptArg(int& offset); // Moved to public

    // Data
    std::vector<int16_t> m_eventScripts; // kdef.grp (viewed as int16)
    std::vector<int32_t> m_eventIndices; // kdef.idx

    std::vector<uint8_t> m_talkData; // talk.grp
    std::vector<int32_t> m_talkIndices; // talk.idx
    
    // Name Data
    std::vector<uint8_t> m_nameData; // name.grp
    std::vector<int32_t> m_nameIndices; // name.idx
    
    // Helper to load name
    std::string GetNameFromData(int nameNum);

    // Context
    int m_currentSceneId = -1;
    int m_currentEventId = -1;
};

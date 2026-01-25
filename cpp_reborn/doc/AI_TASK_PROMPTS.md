# KYS C++ 重制版开发全景文档 (Master Development Doc)

**最后更新**: 2026-01-24
**版本**: Alpha 0.4
**状态**: 核心逻辑修复与工具链完善阶段

**角色**: 高级 C++ 游戏开发工程师 / 架构师
**项目目标**: 将《金庸群侠前传》(Pascal) 重构为 C++ (SDL3)，保持对原版二进制数据文件 (`.grp`, `.idx`) 的 **1:1 兼容性**，并实现现代化的代码架构。

---

## 1. 项目状态总览 (Status Overview)

### 核心原则
1.  **二进制兼容**: 直接读取原版资源，不修改文件结构。
2.  **逻辑保真**: 战斗公式、事件指令、AI 逻辑严格对齐 Pascal 源码。
3.  **架构现代化**: 单例模式管理子系统 (`GameManager`, `SceneManager` 等)，分离逻辑与渲染。

### 📅 最近重大更新 (Changelog)
*   **2026-01-24 (Critical Fixes & Tools)**:
    *   **事件系统修复**: 修正了 `Instruct_ModifyEvent` 中的自动触发逻辑。增加了距离检测，防止修改远端事件（如场景出口）属性时错误地立即触发该事件（解决了“大厅对话后直接跳过行走阶段”的问题）。
    *   **开局流程修正**: 将新游戏起始坐标修正为 Scene 0 (38, 38)，并修复了 `InitNewGame` 中摄像机未同步的问题。
    *   **资源加载优化**: 实现了 `GameManager::getHead` 和 `UIManager` 的**懒加载机制**，修复了对话框头像不显示和物品栏图标/简介缺失的问题。
    *   **调试工具链**: 新增 `save_inspector.exe` (存档/全剧数据查看器) 和 `analyze_data.exe` (事件定义分析器)，用于逆向分析 `.grp` 文件。
*   **2026-01-23 (UI & Event System)**:
    *   **ESC 菜单重构**: 初步实现环形菜单。
    *   **多帧动画**: 实现了 `instruct_23`。

---

## 2. 详细开发任务清单 (Task List)

### 🔴 Phase 1: 核心功能完善 (Priority: High)

#### 🚨 C-00: ESC 菜单视觉修复 (Critical)
*   **当前问题**: ESC 菜单的 6 个功能按钮（武、态、系、友、技、具）贴图调用逻辑错误。
*   **技术细节**: `Background.Pic` Index 6 包含 **12 个图块** (6按钮 × 2状态)。
*   **Prompt 指令**: "修复 `UIManager::RenderMenuSystem` 中 Index 6 贴图的切片逻辑。必须严格参照 Pascal 源码 (`kys_engine.pas` -> `NewMenuEsc`)，根据 Menu Index 计算 UV 坐标。"
*   **验收标准**: 环形菜单按钮随光标正确切换高亮状态。

#### C-01: UI 系统补全 (进行中)
*   **当前状态**:
    *   [x] 物品列表显示（图标 + 简介）。
    *   [ ] **物品使用/装备逻辑**: 点击物品后的效果（吃药、装备）尚未完全连接到角色属性。
    *   [ ] **系统菜单**: 存读档界面 UI 已有，但需验证 `SaveGame` / `LoadGame` 的二进制写入是否正确。
*   **Prompt 指令**: "完善 `InventoryUI` 的交互逻辑，实现 `UseItem` 和 `EquipItem`。完善 `SystemMenu` 的存档写入功能，确保生成的 `R*.grp` 能被 `save_inspector` 正确读取。"

#### C-02: 战斗系统交互 (Pending)
*   **当前状态**: 核心逻辑（移动、伤害、AI）已完成。**缺失玩家交互层**。
*   **Prompt 指令**: "实现 `BattleManager` 的玩家交互层。完成 `BattleMenu`，处理玩家的移动选择、攻击目标选择、药品使用等输入。连接现有的 `MoveRole` 和 `Attack` 接口。"
*   **验收标准**: 玩家可手动控制角色完成一场战斗。

#### C-03: 事件系统增强 (已修复衔接问题)
*   **当前状态**: 基础指令、镜头控制、事件链逻辑已修复。
*   **待办事项**:
    *   **多帧动画同步**: 检查 `instruct_23` 等动画指令是否会阻塞主线程导致无响应。
    *   **指令补全**: 对照 `kys_event.pas` 检查是否有遗漏的低频指令。
*   **Prompt 指令**: "继续完善 `EventManager`，重点关注动画指令的非阻塞实现。"

#### C-04: 视觉特效 (VFX)
*   **当前状态**: 暂无战斗特效。
*   **Prompt 指令**: "在 `BattleManager` 中实现武功特效渲染。加载 `.eft` 资源，根据攻击范围播放动画序列。"

---

### 🟡 Phase 2: 架构优化 (Priority: Medium)

#### O-01: 输入系统重构
*   **当前状态**: `UIManager` 中存在大量 `SDL_PollEvent` 的局部循环，容易导致逻辑割裂。
*   **Prompt 指令**: "建立统一的 `InputManager`，废弃局部输入循环，使用状态机管理输入。"

#### O-02: 音频系统迁移
*   **当前状态**: 依赖 Windows MCI。
*   **Prompt 指令**: "将音频系统迁移到 `SDL_mixer`，移除 `winmm.lib` 依赖。"

---

## 3. 常用调试工具 (New)

*   **save_inspector.exe**:
    *   用途: 查看 `save/` 目录下 `.grp` 文件的内部数据（角色属性、物品、全局状态）。
    *   用法: `save_inspector.exe [save_dir]`
*   **analyze_data.exe**:
    *   用途: 解析 `alldef.grp` (事件定义) 和 `allsin.grp` (地图定义)。
    *   用法: 交互式选择分析对象。
*   **dump_events.exe**:
    *   用途: 打印指定 Event ID 的指令序列。

---

## 4. 技术规范速查 (Reference)

### 4.1 坐标系统
*   **Scene 0 (圣堂)** 起始坐标: `(38, 38)`
*   **镜头逻辑**: `setMainMapPosition(x, y)` 会同步更新 `m_mainMapX/Y` 和 `m_cameraX/Y`。

### 4.2 指令修正记录
*   **Instruct_ModifyEvent**: 仅当事件位于玩家当前坐标时，或为无坐标的系统事件时，才允许脚本变更触发自动运行。禁止远程修改导致的瞬间传送/触发。

---

**执行提示**: 开发前请运行 `save_inspector` 确认数据状态，修改代码后更新本文件。

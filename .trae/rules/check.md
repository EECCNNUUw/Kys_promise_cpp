# KYS Project Refactoring Consistency Rules & Skeleton

This document serves as the **Single Source of Truth** for the KYS Pascal-to-C++ refactoring project. All code generation and modification must adhere to these guidelines to ensure logical consistency across different conversational contexts.

## 1. Core Principles

1.  **Binary Compatibility First**: The C++ implementation MUST be able to read/write the original game's data files (`.grp`, `.idx`, etc.) without modification.
    -   **Rule**: All game entities (`Role`, `Item`, `Magic`, `Scene`) must inherit from `GameObject` and maintain an internal `int16_t` buffer that mirrors the Pascal memory layout exactly.
    -   **Prohibition**: Do not add new member variables to these classes that are expected to be serialized, unless they are separate "runtime-only" properties.

2.  **Logic Fidelity**: The game logic must replicate the original Pascal behavior unless explicitly improved (e.g., pathfinding optimization).
    -   **Rule**: When porting a function (e.g., damage calculation), first analyze the Pascal source (`kys_battle.pas`), then implement the equivalent C++ logic. Do not guess formulas.

3.  **No Hallucinations**: Do not invent features or file formats that do not exist in the original project unless requested. Stick to the files found in `d:\program\misc\kys-promise-main`.

4.  **Toolchain Restrictions**:
    -   **Rule**: Use `msbuild` or standard static analysis tools compatible with Windows permissions.
    -   **Prohibition**: **DO NOT use `g++`** for compilation or syntax checking, as it is blocked by corporate security software.

## 2. Logical Skeleton & Architecture

### 2.1. Class Hierarchy
All data-driven entities derive from a common base to handle serialization.

```cpp
class GameObject {
protected:
    std::vector<int16_t> m_data; // The raw binary data
public:
    // ... accessors that map indices to properties ...
};

class Role : public GameObject { ... };  // Wraps TRole (91 ints)
class Item : public GameObject { ... };  // Wraps TItem (95 ints)
class Magic : public GameObject { ... }; // Wraps TMagic
class Scene : public GameObject { ... }; // Wraps TScene
```

### 2.2. Global State Management
Pascal's global variables (in `kys_main.pas`) are consolidated into a Singleton `GameManager`.

-   **Pascal**: `var RRole: array of TRole;`
-   **C++**: `GameManager::getInstance().getRole(id)`
-   **Pascal**: `var CurrentScene: integer;`
-   **C++**: `GameManager::getInstance().getCurrentSceneId()`

### 2.3. Subsystems
-   **Graphics**: `GraphicsUtils` (wraps SDL3, replaces `kys_gfx.pas`)
-   **Battle**: `BattleManager` (encapsulates `kys_battle.pas` logic)
-   **Event**: `EventManager` (encapsulates `kys_event.pas` logic)
-   **UI**: `UIManager` (New system, but logic derived from `kys_engine.pas` drawing routines)

## 3. Type Mapping Standards

Strictly follow this mapping table to ensure data aligns with Pascal binaries.

| Pascal Type | C++ Type | Size (Bytes) | Notes |
| :--- | :--- | :--- | :--- |
| `smallint` | `int16_t` | 2 | **CRITICAL**. Primary data unit. |
| `integer` | `int32_t` | 4 | Use for loop counters, memory offsets. |
| `word` / `uint16` | `uint16_t` | 2 | Unsigned data. |
| `byte` | `uint8_t` | 1 | Byte data. |
| `boolean` | `bool` | 1 | Careful with size in structs; usually not in binary data. |
| `ansichar` array | `std::string` | N/A | Convert on access. Raw data stays as `int16` array. |
| `PChar` / `PAnsiChar` | `const char*` | - | For SDL API calls. |

## 4. Porting Workflow

1.  **Identify**: Locate the Pascal function to port (e.g., `kys_battle.pas` -> `AutoBattle`).
2.  **Analyze**: Understand the variable usage and control flow. Note any global variable dependencies.
3.  **Map**: Determine which C++ Manager/Class should own this logic.
4.  **Implement**: Write the C++ code using the defined Type Mappings.
5.  **Verify**: Compare the logic flow. If possible, write a unit test or log output to verify against expected behavior.

## 5. File Structure Enforcement

-   `cpp_reborn/include/`: ONLY header files. Interface definitions.
-   `cpp_reborn/src/`: ONLY implementation files.
-   `cpp_reborn/include/GameTypes.h`: Global typedefs and constants.
-   `cpp_reborn/include/GameObject.h`: The core base class.


## 7. Game Logic Specifics
-   **Starting Scene**: This project uses a modified starting scene. Always assume **Scene 0 (Temple)** is the correct starting location, unless `ranger.grp` header explicitly overrides it. Do not hardcode Scene 70.
-   **Coordinates**: When starting in Scene 0, the default coordinates should be `(38, 38)`.

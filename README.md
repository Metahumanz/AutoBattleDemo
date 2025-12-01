# AutoBattleDemo

## UE4 C++ Final Project by Team 657220

A minimal auto-battler prototype built with **Unreal Engine 4.24** and **C++**. Units autonomously navigate and attack enemy structures using UE4’s AI and NavMesh systems.

> ✅ Meets all course requirements:  
> - Core logic implemented in **C++** (with Blueprint integration)  
> - Developed collaboratively using **Git**  
> - Uses **OOP**, **STL**, and modern C++ features (e.g., `auto`, `nullptr`)

---

## Gameplay Overview

- Press the **"Spawn Warrior"** button to deploy units.  
- Units automatically pathfind to the enemy base using **NavMesh**.  
- On reaching attack range, they deal damage over time.  
- Destroy the red enemy base to win.

This is a technical prototype focused on core systems, not full gameplay depth.

---

## Technical Implementation

### Core Architecture
- Custom `AGameMode` spawns all game objects at runtime  
- `ABase` class provides health and destruction logic  
- No actors are placed directly in the level — everything is dynamically generated

### AI & Navigation
- Units use `AAIController` and `UNavigationSystemV1` for autonomous movement  
- NavMesh Bounds Volume enables pathfinding

### Integration
- UMG UI calls C++ functions via `UFUNCTION(BlueprintCallable)`  
- Blueprint classes inherit from C++ base types for extensibility

---

## How to Run

### Requirements
- Windows 10/11  
- Visual Studio 2019 (with C++ desktop workload)  
- Unreal Engine **4.24** (installed via Epic Launcher)

### Steps
1. Clone this repository  
2. Right-click `AutoBattleDemo.uproject` → **Generate Visual Studio project files**  
3. Open the generated `.sln` in Visual Studio  
4. Set configuration to **Development Editor**, platform to **Win64**  
5. Press **Ctrl+F5** to compile and launch the editor  
6. Open `Content/Maps/Map_Main.umap` and click **Play**

---

## Project Structure

```text
AutoBattleDemo/
├── Source/AutoBattleDemo/     # C++ source (GameMode, ABase, spawning logic)
├── Content/Blueprints/        # Unit and UI Blueprints
├── Content/Maps/Map_Main.umap # Empty scene (all content spawned at runtime)
└── AutoBattleDemo.uproject
```



---

## License

For educational purposes only.  

© 2025 Team 657220
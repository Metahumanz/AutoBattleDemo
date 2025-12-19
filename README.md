# AutoBattleDemo

## Team 657220 UE4 C++ 期末项目

这是一个基于 **Unreal Engine 4.24** 和 **C++** 构建的极简自动对战（Auto-battler）原型。单位利用 UE4 的 AI 和 NavMesh 系统自动寻路并攻击敌方建筑。

> ✅ 满足所有课程要求：
> - 核心逻辑由 **C++** 实现（并与蓝图集成）
> - 使用 **Git** 进行协同开发
> - 使用了 **OOP**（面向对象编程）、**STL** 以及现代 C++ 特性（如 `auto`, `nullptr`）

---

## 游戏玩法概览

- 点击 **“Spawn Warrior” (生成战士)** 按钮部署单位。
- 单位使用 **NavMesh**（导航网格）自动寻路至敌方基地。
- 到达攻击范围后，它们会持续造成伤害。
- 摧毁红色的敌方基地即可获胜。

这是一个专注于核心系统的技术原型，不包含完整的游戏深度。

---

## 技术实现

### 核心架构
- 自定义 `AGameMode` 在运行时生成所有游戏对象
- `ABase` 类提供生命值和销毁逻辑
- 关卡中不直接放置 Actor —— 所有内容均为动态生成

### AI 与导航
- 单位使用 `AAIController` 和 `UNavigationSystemV1` 实现自主移动
- 使用 NavMesh Bounds Volume（导航网格边界体积）启用寻路功能

### 集成
- UMG UI 通过 `UFUNCTION(BlueprintCallable)` 调用 C++ 函数
- 蓝图类继承自 C++ 基类以实现扩展性

---

## 运行指南

### 环境要求
- Windows 10/11
- Visual Studio 2019（需安装 C++ 桌面开发工作负载）
- Unreal Engine **4.24**（通过 Epic Launcher 安装）

### 步骤
1. 克隆此仓库
2. 右键点击 `AutoBattleDemo.uproject` → 选择 **Generate Visual Studio project files**（生成 VS 项目文件）
3. 在 Visual Studio 中打开生成的 `.sln` 文件
4. 将解决方案配置设置为 **Development Editor**，平台设置为 **Win64**
5. 按 **Ctrl+F5** 编译并启动编辑器
6. 打开 `Content/Maps/Map_Main.umap` 并点击 **Play**（运行）

---

## 项目结构

```text
AutoBattleDemo/
├── Source/AutoBattleDemo/     # C++ 源码 (GameMode, ABase, 生成逻辑)
├── Content/Blueprints/        # 单位和 UI 蓝图
├── Content/Maps/Map_Main.umap # 空场景 (所有内容在运行时生成)
└── AutoBattleDemo.uproject
```

---

## 许可证

仅供教育用途。

© 2025 Team 657220
// RTSCoreTypes.h
#pragma once
#include "CoreMinimal.h"
#include "RTSCoreTypes.generated.h"

// 游戏阶段状态
UENUM(BlueprintType)
enum class EGameState : uint8
{
    Preparation, // 备战阶段：玩家买兵、摆阵
    Battle,      // 战斗阶段：自动寻路、攻击
    Victory,     // 结算：胜利
    Defeat       // 结算：失败
};

// 队伍阵营
UENUM(BlueprintType)
enum class ETeam : uint8
{
    Player,
    Enemy
};

// 兵种类型（用于商店购买）
UENUM(BlueprintType)
enum class EUnitType : uint8
{
    Soldier, // 近战
    Archer,  // 远程
    Tank     // 肉盾
};
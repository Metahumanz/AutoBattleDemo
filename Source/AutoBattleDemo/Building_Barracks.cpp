#include "Building_Barracks.h"
#include "RTSGameInstance.h"
#include "Kismet/GameplayStatics.h"

ABuilding_Barracks::ABuilding_Barracks()
{
    BuildingType = EBuildingType::Barracks; // 确保 CoreTypes 里有这个枚举
    MaxHealth = 800.0f;
    TeamID = ETeam::Player;

    // 默认每个兵营提供 5 人口
    PopulationBonus = 5;
}

void ABuilding_Barracks::BeginPlay()
{
    Super::BeginPlay();

    // 只有玩家的兵营才增加玩家的人口上限
    if (TeamID == ETeam::Player)
    {
        URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
        if (GI)
        {
            GI->MaxPopulation += PopulationBonus;

            UE_LOG(LogTemp, Log, TEXT("Barracks Built! Max Pop +%d. Current Max: %d"),
                PopulationBonus, GI->MaxPopulation);
        }
    }
}

void ABuilding_Barracks::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    // 只有当它是被“销毁”时才扣除 (包括被Remove工具删除，或被敌人打死)
    // EndPlayReason::Destroyed 是最常见的情况
    if (TeamID == ETeam::Player)
    {
        URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
        if (GI)
        {
            // 扣除上限，但最小不低于初始值(比如20)或者0
            // 这里简单处理，直接减
            GI->MaxPopulation = FMath::Max(0, GI->MaxPopulation - PopulationBonus);

            UE_LOG(LogTemp, Log, TEXT("Barracks Destroyed! Max Pop -%d. Current Max: %d"),
                PopulationBonus, GI->MaxPopulation);
        }
    }
}
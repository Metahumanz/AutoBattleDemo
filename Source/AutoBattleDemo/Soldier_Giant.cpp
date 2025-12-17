#include "Soldier_Giant.h"
#include "Building_Defense.h"
#include "BaseBuilding.h"
#include "Kismet/GameplayStatics.h"

ASoldier_Giant::ASoldier_Giant()
{
    // 巨人属性：超高血量，慢速，近战
    MaxHealth = 500.0f;     // 血牛！
    AttackRange = 150.0f;
    Damage = 30.0f;         // 高攻击
    MoveSpeed = 150.0f;     // 很慢
    AttackInterval = 1.5f;
}

void ASoldier_Giant::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[Giant] %s spawned | HP: %f | Damage: %f"),
        *GetName(), MaxHealth, Damage);
}

AActor* ASoldier_Giant::FindClosestEnemyBuilding()
{
    // 第一步：优先寻找防御塔
    TArray<AActor*> AllDefenses;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding_Defense::StaticClass(), AllDefenses);

    AActor* ClosestDefense = nullptr;
    float ClosestDefenseDistance = FLT_MAX;

    for (AActor* Actor : AllDefenses)
    {
        ABuilding_Defense* Defense = Cast<ABuilding_Defense>(Actor);

        if (Defense &&
            Defense->TeamID != this->TeamID &&
            Defense->CurrentHealth > 0)
        {
            float Distance = FVector::Dist(GetActorLocation(), Defense->GetActorLocation());

            // 优先返回攻击范围内的防御塔
            if (Distance <= AttackRange)
            {
                UE_LOG(LogTemp, Warning, TEXT("[Giant] %s targeting defense tower: %s (in range)"),
                    *GetName(), *Defense->GetName());
                return Defense;
            }

            if (Distance < ClosestDefenseDistance)
            {
                ClosestDefenseDistance = Distance;
                ClosestDefense = Defense;
            }
        }
    }

    // 如果找到防御塔，返回最近的
    if (ClosestDefense)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Giant] %s targeting defense tower: %s (distance: %f)"),
            *GetName(), *ClosestDefense->GetName(), ClosestDefenseDistance);
        return ClosestDefense;
    }

    // 第二步：没有防御塔了，找普通建筑
    TArray<AActor*> AllBuildings;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBuilding::StaticClass(), AllBuildings);

    AActor* ClosestBuilding = nullptr;
    float ClosestDistance = FLT_MAX;

    for (AActor* Actor : AllBuildings)
    {
        ABaseBuilding* Building = Cast<ABaseBuilding>(Actor);

        // 排除已经找过的防御塔
        if (Building &&
            !Building->IsA(ABuilding_Defense::StaticClass()) &&
            Building->TeamID != this->TeamID &&
            Building->bIsTargetable &&
            Building->CurrentHealth > 0)
        {
            float Distance = FVector::Dist(GetActorLocation(), Building->GetActorLocation());

            if (Distance <= AttackRange)
            {
                return Building;
            }

            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                ClosestBuilding = Building;
            }
        }
    }

    if (ClosestBuilding)
    {
        UE_LOG(LogTemp, Log, TEXT("[Giant] %s targeting normal building: %s"),
            *GetName(), *ClosestBuilding->GetName());
    }

    return ClosestBuilding;
}
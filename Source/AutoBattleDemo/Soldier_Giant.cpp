#include "Soldier_Giant.h"
#include "Building_Defense.h"
#include "BaseBuilding.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h" // 必须引用

ASoldier_Giant::ASoldier_Giant()
{
    UnitType = EUnitType::Giant;
    MaxHealth = 500.0f;
    AttackRange = 150.0f;
    Damage = 30.0f;
    MoveSpeed = 150.0f;
    AttackInterval = 1.5f;
}

void ASoldier_Giant::BeginPlay()
{
    Super::BeginPlay();
}

// 重写：优先寻找防御塔，且使用【表面距离】计算
AActor* ASoldier_Giant::FindClosestTarget()
{
    // 获取所有建筑 (包括塔和普通建筑)
    TArray<AActor*> AllBuildings;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBuilding::StaticClass(), AllBuildings);

    AActor* ClosestDefense = nullptr;
    float ClosestDefenseDist = FLT_MAX;

    AActor* ClosestBuilding = nullptr;
    float ClosestBuildingDist = FLT_MAX;

    for (AActor* Actor : AllBuildings)
    {
        ABaseBuilding* Building = Cast<ABaseBuilding>(Actor);

        // 过滤：必须是敌人，且活着，且可被攻击
        if (Building && Building->TeamID != this->TeamID && Building->CurrentHealth > 0 && Building->bIsTargetable)
        {
            // 计算表面距离 (Surface Distance)
            float DistToSurface = FLT_MAX;
            UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Building->GetRootComponent());
            if (!Prim) Prim = Building->FindComponentByClass<UStaticMeshComponent>();

            if (Prim)
            {
                FVector ClosestPt;
                Prim->GetClosestPointOnCollision(GetActorLocation(), ClosestPt);
                ClosestPt.Z = GetActorLocation().Z;
                DistToSurface = FVector::Dist(GetActorLocation(), ClosestPt);
            }
            else
            {
                DistToSurface = FVector::Dist(GetActorLocation(), Building->GetActorLocation());
            }
            
            // 判断类型
            if (Building->IsA(ABuilding_Defense::StaticClass()))
            {
                // 是防御塔
                if (DistToSurface <= AttackRange) return Building; // 脸上有塔，直接打

                if (DistToSurface < ClosestDefenseDist)
                {
                    ClosestDefenseDist = DistToSurface;
                    ClosestDefense = Building;
                }
            }
            else
            {
                // 是普通建筑
                if (DistToSurface < ClosestBuildingDist)
                {
                    ClosestBuildingDist = DistToSurface;
                    ClosestBuilding = Building;
                }
            }
        }
    }

    // 决策优先级：有塔打塔，没塔打房
    if (ClosestDefense)
    {
        // UE_LOG(LogTemp, Log, TEXT("[Giant] Targeting Defense: %s"), *ClosestDefense->GetName());
        return ClosestDefense;
    }

    // UE_LOG(LogTemp, Log, TEXT("[Giant] Targeting Building: %s"), *ClosestBuilding->GetName());
    return ClosestBuilding;
}
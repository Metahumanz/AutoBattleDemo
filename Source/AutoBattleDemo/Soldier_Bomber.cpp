#include "Soldier_Bomber.h"
#include "BaseBuilding.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ASoldier_Bomber::ASoldier_Bomber()
{
    // 炸弹人属性：低血量，高爆发
    MaxHealth = 50.0f;
    AttackRange = 100.0f;   // 必须走很近才能引爆
    Damage = 0.0f;          // 普通攻击无伤害
    MoveSpeed = 350.0f;     // 最快！
    AttackInterval = 0.0f;  // 一次性攻击

    ExplosionRadius = 300.0f;
    ExplosionDamage = 200.0f;
}

void ASoldier_Bomber::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[Bomber] %s spawned | ExplosionDamage: %f | Radius: %f"),
        *GetName(), ExplosionDamage, ExplosionRadius);
}

AActor* ASoldier_Bomber::FindClosestEnemyBuilding()
{
    // 优先寻找墙（BuildingType == Wall）
    TArray<AActor*> AllBuildings;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBuilding::StaticClass(), AllBuildings);

    AActor* ClosestWall = nullptr;
    float ClosestWallDistance = FLT_MAX;

    AActor* ClosestOther = nullptr;
    float ClosestOtherDistance = FLT_MAX;

    for (AActor* Actor : AllBuildings)
    {
        ABaseBuilding* Building = Cast<ABaseBuilding>(Actor);

        if (Building &&
            Building->TeamID != this->TeamID &&
            Building->CurrentHealth > 0)
        {
            float Distance = FVector::Dist(GetActorLocation(), Building->GetActorLocation());

            // 优先选择墙
            if (Building->BuildingType == EBuildingType::Wall)
            {
                if (Distance <= AttackRange)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[Bomber] %s found wall in range: %s"),
                        *GetName(), *Building->GetName());
                    return Building;
                }

                if (Distance < ClosestWallDistance)
                {
                    ClosestWallDistance = Distance;
                    ClosestWall = Building;
                }
            }
            else
            {
                // 备选：其他建筑
                if (Distance < ClosestOtherDistance)
                {
                    ClosestOtherDistance = Distance;
                    ClosestOther = Building;
                }
            }
        }
    }

    // 优先返回墙
    if (ClosestWall)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bomber] %s targeting wall: %s"),
            *GetName(), *ClosestWall->GetName());
        return ClosestWall;
    }

    // 没有墙了，炸其他建筑
    if (ClosestOther)
    {
        UE_LOG(LogTemp, Log, TEXT("[Bomber] %s no walls found, targeting: %s"),
            *GetName(), *ClosestOther->GetName());
    }

    return ClosestOther;
}

void ASoldier_Bomber::PerformAttack()
{
    if (!CurrentTarget)
    {
        CurrentState = EUnitState::Idle;
        return;
    }

    ABaseGameEntity* TargetEntity = Cast<ABaseGameEntity>(CurrentTarget);
    if (!TargetEntity || TargetEntity->CurrentHealth <= 0)
    {
        CurrentTarget = nullptr;
        CurrentState = EUnitState::Idle;
        return;
    }

    float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());

    if (Distance > AttackRange)
    {
        // 还没到攻击距离，继续移动
        RequestPathToTarget();
        if (PathPoints.Num() > 0)
        {
            CurrentState = EUnitState::Moving;
        }
        return;
    }

    // 到达攻击距离，执行自爆！
    UE_LOG(LogTemp, Error, TEXT("[Bomber] %s EXPLODING!!!"), *GetName());
    SuicideAttack();
}

void ASoldier_Bomber::SuicideAttack()
{
    FVector ExplosionCenter = GetActorLocation();

    // 绘制爆炸范围（调试）
    DrawDebugSphere(GetWorld(), ExplosionCenter, ExplosionRadius,
        16, FColor::Orange, false, 3.0f, 0, 5.0f);

    // 对范围内所有建筑造成伤害
    TArray<AActor*> AllBuildings;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBuilding::StaticClass(), AllBuildings);

    int32 HitCount = 0;
    for (AActor* Actor : AllBuildings)
    {
        ABaseBuilding* Building = Cast<ABaseBuilding>(Actor);

        if (Building &&
            Building->TeamID != this->TeamID &&
            Building->CurrentHealth > 0)
        {
            float Distance = FVector::Dist(ExplosionCenter, Building->GetActorLocation());

            if (Distance <= ExplosionRadius)
            {
                // 应用伤害
                FDamageEvent DamageEvent;
                Building->TakeDamage(ExplosionDamage, DamageEvent, nullptr, this);
                HitCount++;

                UE_LOG(LogTemp, Warning, TEXT("[Bomber] %s hit %s for %f damage!"),
                    *GetName(), *Building->GetName(), ExplosionDamage);
            }
        }
    }

    UE_LOG(LogTemp, Error, TEXT("[Bomber] %s explosion hit %d buildings!"), *GetName(), HitCount);

    // 如果炸毁了墙，通知 GridManager 更新网格
    ABaseBuilding* TargetBuilding = Cast<ABaseBuilding>(CurrentTarget);
    if (TargetBuilding && TargetBuilding->BuildingType == EBuildingType::Wall)
    {
        if (GridManagerRef)
        {
            FVector WallPos = TargetBuilding->GetActorLocation();
            // 假设 GridManager 有 UpdateTileStatus 方法
            // GridManagerRef->SetTileBlocked(WallPos.X, WallPos.Y, false);

            UE_LOG(LogTemp, Warning, TEXT("[Bomber] Wall destroyed, grid should update at %s"),
                *WallPos.ToString());
        }
    }

    // 自爆后销毁自己
    Destroy();
}
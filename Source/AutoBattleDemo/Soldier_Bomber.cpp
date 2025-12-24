#include "Soldier_Bomber.h"
#include "BaseBuilding.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystem.h"

ASoldier_Bomber::ASoldier_Bomber()
{
    UnitType = EUnitType::Bomber;
    MaxHealth = 50.0f;
    AttackRange = 100.0f; // 炸弹人手短，要贴近
    Damage = 0.0f;
    MoveSpeed = 350.0f;
    AttackInterval = 0.0f;

    ExplosionRadius = 300.0f;
    ExplosionDamage = 200.0f;
}

void ASoldier_Bomber::BeginPlay()
{
    Super::BeginPlay();
}

// 重写：优先寻找墙，使用【表面距离】
AActor* ASoldier_Bomber::FindClosestTarget()
{
    TArray<AActor*> AllBuildings;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBuilding::StaticClass(), AllBuildings);

    AActor* ClosestWall = nullptr;
    float ClosestWallDist = FLT_MAX;

    AActor* ClosestOther = nullptr;
    float ClosestOtherDist = FLT_MAX;

    for (AActor* Actor : AllBuildings)
    {
        ABaseBuilding* Building = Cast<ABaseBuilding>(Actor);

        if (Building && Building->TeamID != this->TeamID && Building->CurrentHealth > 0 && Building->bIsTargetable)
        {
            // 计算表面距离
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
            
            // 优先找墙
            if (Building->BuildingType == EBuildingType::Wall)
            {
                if (DistToSurface <= AttackRange) return Building; // 脸上有墙，直接炸

                if (DistToSurface < ClosestWallDist)
                {
                    ClosestWallDist = DistToSurface;
                    ClosestWall = Building;
                }
            }
            else
            {
                if (DistToSurface < ClosestOtherDist)
                {
                    ClosestOtherDist = DistToSurface;
                    ClosestOther = Building;
                }
            }
        }
    }

    if (ClosestWall) return ClosestWall;
    return ClosestOther;
}

// 执行自爆前的判定
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

    // 再次计算表面距离 (防止还没走到就炸了)
    float DistToSurface = FLT_MAX;
    UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(CurrentTarget->GetRootComponent());
    if (!Prim) Prim = CurrentTarget->FindComponentByClass<UStaticMeshComponent>();

    if (Prim)
    {
        FVector ClosestPt;
        Prim->GetClosestPointOnCollision(GetActorLocation(), ClosestPt);
        ClosestPt.Z = GetActorLocation().Z;
        DistToSurface = FVector::Dist(GetActorLocation(), ClosestPt);
    }
    else
    {
        DistToSurface = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
    }
    
    // 如果距离还不够近 (加一点点缓冲)，继续走
    if (DistToSurface > (AttackRange + 20.0f))
    {
        RequestPathToTarget();
        if (PathPoints.Num() > 0) CurrentState = EUnitState::Moving;
        return;
    }

    // 距离够了，BOOM！
    UE_LOG(LogTemp, Warning, TEXT("[Bomber] %s Reached target %s (Dist: %f), EXPLODING!"),
        *GetName(), *CurrentTarget->GetName(), DistToSurface);

    SuicideAttack();
}

void ASoldier_Bomber::SuicideAttack()
{
    FVector ExplosionCenter = GetActorLocation();

    // 1. 特效
    if (ExplosionVFX)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionVFX, ExplosionCenter, FRotator::ZeroRotator, FVector(3.0f));
    }

    // 2. 范围伤害
    TArray<AActor*> AllBuildings;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBuilding::StaticClass(), AllBuildings);

    int32 HitCount = 0;
    TArray<ABaseBuilding*> DestroyedWalls;

    for (AActor* Actor : AllBuildings)
    {
        ABaseBuilding* Building = Cast<ABaseBuilding>(Actor);
        // 炸建筑，且是敌人
        if (Building && Building->TeamID != this->TeamID && Building->CurrentHealth > 0)
        {
            float Distance = FVector::Dist(ExplosionCenter, Building->GetActorLocation());

            // 只要在爆炸半径内 (注意：这里用中心距离判定爆炸范围是合理的，因为爆炸是球形的)
            if (Distance <= ExplosionRadius)
            {
                float HealthBefore = Building->CurrentHealth;

                FDamageEvent DamageEvent;
                // 对墙造成 5 倍 伤害 (炸弹人特性)
                float FinalDamage = ExplosionDamage;
                if (Building->BuildingType == EBuildingType::Wall) FinalDamage *= 5.0f;

                Building->TakeDamage(FinalDamage, DamageEvent, nullptr, this);
                HitCount++;

                // 记录炸毁的墙
                if (Building->BuildingType == EBuildingType::Wall &&
                    HealthBefore > 0 && Building->CurrentHealth <= 0)
                {
                    DestroyedWalls.Add(Building);
                }
            }
        }
    }

    // 3. 通知 GridManager 解锁格子
    if (DestroyedWalls.Num() > 0 && GridManagerRef)
    {
        for (ABaseBuilding* Wall : DestroyedWalls)
        {
            if (Wall->GridX >= 0 && Wall->GridY >= 0)
            {
                GridManagerRef->SetTileBlocked(Wall->GridX, Wall->GridY, false);
            }
        }
    }

    // 4. 销毁自己
    Destroy();
}
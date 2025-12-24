#include "Soldier_Archer.h"
#include "RTSProjectile.h" 
#include "BaseBuilding.h"
#include "Components/StaticMeshComponent.h" // 必须引用

ASoldier_Archer::ASoldier_Archer()
{
    // 设置兵种类型
    UnitType = EUnitType::Archer;

    // 弓箭手属性
    MaxHealth = 80.0f;
    AttackRange = 500.0f;
    Damage = 12.0f;
    MoveSpeed = 200.0f;
    AttackInterval = 1.2f;
}

void ASoldier_Archer::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[Archer] %s spawned | HP: %f | Range: %f"),
        *GetName(), MaxHealth, AttackRange);
}

void ASoldier_Archer::PerformAttack()
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

    // 使用表面距离判定 (Surface Distance)
    float DistToSurface = FLT_MAX;

    // 1. 尝试找目标的物理组件
    UPrimitiveComponent* TargetPrim = Cast<UPrimitiveComponent>(CurrentTarget->GetRootComponent());
    if (!TargetPrim) TargetPrim = CurrentTarget->FindComponentByClass<UStaticMeshComponent>();

    if (TargetPrim)
    {
        FVector ClosestPt;
        // 获取最近点
        TargetPrim->GetClosestPointOnCollision(GetActorLocation(), ClosestPt);
        ClosestPt.Z = GetActorLocation().Z; // 抹平高度
        DistToSurface = FVector::Dist(GetActorLocation(), ClosestPt);
    }
    else
    {
        // 保底：中心距离
        DistToSurface = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
    }

    // 检查距离：如果超出了 (射程 + 缓冲)，重新追击
    if (DistToSurface > (AttackRange + 50.0f))
    {
        RequestPathToTarget();
        if (PathPoints.Num() > 0)
        {
            CurrentState = EUnitState::Moving;
        }
        return;
    }

    // 执行攻击
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastAttackTime >= AttackInterval)
    {
        LastAttackTime = CurrentTime;

        // 生成投射物
        if (ProjectileClass)
        {
            FVector SpawnLoc = GetActorLocation() + FVector(0, 0, 100); // 从头顶发射
            FRotator SpawnRot = (CurrentTarget->GetActorLocation() - SpawnLoc).Rotation();

            ARTSProjectile* Arrow = GetWorld()->SpawnActor<ARTSProjectile>(ProjectileClass, SpawnLoc, SpawnRot);
            if (Arrow)
            {
                Arrow->Initialize(CurrentTarget, Damage, this);
            }
        }
        else
        {
            // 保底直接伤害
            FDamageEvent DamageEvent;
            CurrentTarget->TakeDamage(Damage, DamageEvent, nullptr, this);
        }

        UE_LOG(LogTemp, Log, TEXT("Archer Fired Arrow!"));
    }
}
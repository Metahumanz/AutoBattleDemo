#include "BaseUnit.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h" // 用于遍历 Actor

ABaseUnit::ABaseUnit()
{
    PrimaryActorTick.bCanEverTick = true;

    // 默认数值
    MaxHealth = 100.0f;
    AttackRange = 150.0f; // 近战距离
    Damage = 10.0f;
    MoveSpeed = 300.0f;
    AttackInterval = 1.0f;

    CurrentState = EUnitState::Idle;
    LastAttackTime = 0.0f;
    CurrentPathIndex = 0;
}

void ABaseUnit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 状态机
    switch (CurrentState)
    {
    case EUnitState::Idle:
        // 如果激活了且没目标，找目标
        if (!CurrentTarget)
        {
            CurrentTarget = FindClosestEnemy();
            if (CurrentTarget)
            {
                RequestPathToTarget();
            }
        }
        break;

    case EUnitState::Moving:
        MoveAlongPath(DeltaTime);
        break;

    case EUnitState::Attacking:
        PerformAttack();
        break;
    }
}

void ABaseUnit::SetUnitActive(bool bActive)
{
    if (bActive)
    {
        CurrentState = EUnitState::Idle;
        // 可以在这里加个特效或动作
    }
}

AActor* ABaseUnit::FindClosestEnemy()
{
    AActor* ClosestActor = nullptr;
    float MinDistSq = FLT_MAX;

    // 遍历所有 BaseGameEntity
    for (TActorIterator<ABaseGameEntity> It(GetWorld()); It; ++It)
    {
        ABaseGameEntity* OtherEntity = *It;
        // 排除自己，排除友军，排除死人
        if (OtherEntity && OtherEntity != this && OtherEntity->TeamID != this->TeamID && OtherEntity->CurrentHealth > 0)
        {
            // 手动计算两个向量距离的平方
            float DistSq = FVector::DistSquared(GetActorLocation(), OtherEntity->GetActorLocation());
            if (DistSq < MinDistSq)
            {
                MinDistSq = DistSq;
                ClosestActor = OtherEntity;
            }
        }
    }
    return ClosestActor;
}

void ABaseUnit::RequestPathToTarget()
{
    if (!CurrentTarget) return;

    // 获取 GridManager
    if (!GridManagerRef)
    {
        GridManagerRef = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
    }

    if (GridManagerRef)
    {
        // 调用成员 A 的寻路
        PathPoints = GridManagerRef->FindPath(GetActorLocation(), CurrentTarget->GetActorLocation());

        if (PathPoints.Num() > 0)
        {
            CurrentPathIndex = 0;
            CurrentState = EUnitState::Moving;
        }
    }
}

void ABaseUnit::MoveAlongPath(float DeltaTime)
{
    // 如果没有路径或目标丢失
    if (PathPoints.Num() == 0 || !CurrentTarget)
    {
        CurrentState = EUnitState::Idle;
        return;
    }

    // 1. 检查是否进入攻击范围
    float DistToTarget = GetDistanceTo(CurrentTarget);
    if (DistToTarget <= AttackRange)
    {
        CurrentState = EUnitState::Attacking;
        return;
    }

    // 2. 获取当前路点
    if (PathPoints.IsValidIndex(CurrentPathIndex))
    {
        FVector TargetPoint = PathPoints[CurrentPathIndex];
        // 忽略 Z 轴差异
        TargetPoint.Z = GetActorLocation().Z;

        // 计算方向
        FVector Direction = (TargetPoint - GetActorLocation()).GetSafeNormal();

        // 手动移动位置 (Manual Movement)
        AddActorWorldOffset(Direction * MoveSpeed * DeltaTime);

        // 旋转朝向
        SetActorRotation(Direction.Rotation());

        // 检查是否到达该路点 (误差 10.0f)
        if (FVector::DistSquared(GetActorLocation(), TargetPoint) < 100.0f)
        {
            CurrentPathIndex++;
            // 如果走完了所有点
            if (CurrentPathIndex >= PathPoints.Num())
            {
                // 重新寻路或者检查攻击
                RequestPathToTarget();
            }
        }
    }
}

void ABaseUnit::PerformAttack()
{
    if (!CurrentTarget)
    {
        CurrentState = EUnitState::Idle;
        return;
    }

    // 检查距离，如果敌人跑了，重新追
    if (GetDistanceTo(CurrentTarget) > AttackRange * 1.2f) // 留一点缓冲
    {
        CurrentState = EUnitState::Moving;
        RequestPathToTarget();
        return;
    }

    // 攻击冷却
    float TimeSeconds = GetWorld()->GetTimeSeconds();
    if (TimeSeconds - LastAttackTime >= AttackInterval)
    {
        LastAttackTime = TimeSeconds;

        // 造成伤害
        UGameplayStatics::ApplyDamage(CurrentTarget, Damage, GetController(), this, UDamageType::StaticClass());

        // UE_LOG(LogTemp, Log, TEXT("Attacked!"));
    }
}
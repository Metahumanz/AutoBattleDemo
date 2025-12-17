#include "Soldier_Archer.h"

ASoldier_Archer::ASoldier_Archer()
{
    // 弓箭手属性：低血量，远程攻击
    MaxHealth = 80.0f;
    AttackRange = 500.0f;   // 远程！
    Damage = 12.0f;
    MoveSpeed = 200.0f;     // 最慢
    AttackInterval = 1.2f;  // 射速稍慢
}

void ASoldier_Archer::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[Archer] %s spawned | HP: %f | Range: %f"),
        *GetName(), MaxHealth, AttackRange);
}

void ASoldier_Archer::PerformAttack()
{
    // 弓箭手攻击逻辑：只要在攻击范围内就停下来开火
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

    // 关键：远程单位只要 Distance <= AttackRange 就停下攻击
    if (Distance > AttackRange)
    {
        // 目标超出范围，重新寻路
        RequestPathToTarget();
        if (PathPoints.Num() > 0)
        {
            CurrentState = EUnitState::Moving;
        }
        return;
    }

    // 在范围内，执行攻击
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastAttackTime >= AttackInterval)
    {
        FDamageEvent DamageEvent;
        CurrentTarget->TakeDamage(Damage, DamageEvent, nullptr, this);
        LastAttackTime = CurrentTime;

        // 面向目标
        FVector Direction = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        if (!Direction.IsNearlyZero())
        {
            FRotator NewRotation = Direction.Rotation();
            NewRotation.Pitch = 0;
            NewRotation.Roll = 0;
            SetActorRotation(NewRotation);
        }

        UE_LOG(LogTemp, Log, TEXT("[Archer] %s shot %s from distance %f!"),
            *GetName(), *CurrentTarget->GetName(), Distance);

        // TODO: 在这里生成箭矢投射物
    }
}
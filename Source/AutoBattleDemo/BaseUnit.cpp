#include "BaseUnit.h"
#include "BaseBuilding.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"

ABaseUnit::ABaseUnit()
{
    PrimaryActorTick.bCanEverTick = true;

    // 1. 创建胶囊体
    CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
    RootComponent = CapsuleComp;
    CapsuleComp->InitCapsuleSize(50.0f, 60.0f);
    CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));

    // 2. 创建模型
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(CapsuleComp);
    MeshComp->SetRelativeLocation(FVector(0.f, 0.f, 0.f)); // 交给蓝图调整

    // 默认属性
    MaxHealth = 100.0f;
    AttackRange = 150.0f;
    Damage = 10.0f;
    MoveSpeed = 300.0f;
    AttackInterval = 1.0f;

    UnitType = EUnitType::Barbarian;
    CurrentState = EUnitState::Idle;
    LastAttackTime = 0.0f;
    CurrentPathIndex = 0;
    CurrentTarget = nullptr;
    GridManagerRef = nullptr;
    bIsActive = false;

    TeamID = ETeam::Player;
    bIsTargetable = true;
}

void ABaseUnit::BeginPlay()
{
    Super::BeginPlay();

    // 1. 获取 GridManager
    if (!GridManagerRef)
    {
        GridManagerRef = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
    }

    // 2. 自动激活逻辑
    FString MapName = GetWorld()->GetMapName();
    if (MapName.Contains("BattleField") && TeamID == ETeam::Enemy)
    {
        SetUnitActive(true);
    }

    // 3. 记录模型初始位置
    if (MeshComp)
    {
        OriginalMeshOffset = MeshComp->GetRelativeLocation();
    }
    bIsLunging = false;

    // 4. 随机化速度 (防止阅兵式行走)
    MoveSpeed *= FMath::RandRange(0.85f, 1.15f);
}

void ABaseUnit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!bIsActive) return;

    // ⬇️⬇️⬇️ [调试] 显示当前状态 ⬇️⬇️⬇️
    /*FString StateName;
    if (UnitType == EUnitType::Barbarian) StateName = "Barbarian";
    else if (UnitType == EUnitType::Bomber) StateName = "Bomber";
    else if (UnitType == EUnitType::Giant) StateName = "Giant";
    else if (UnitType == EUnitType::Archer) StateName = "Archer";
    
    if (CurrentState == EUnitState::Idle) StateName += " IDLE";
    else if (CurrentState == EUnitState::Moving) StateName += " MOVING";
    else if (CurrentState == EUnitState::Attacking) StateName += " ATTACKING";

    
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, StateName);*/
    // ⬆️⬆️⬆️ [调试] ⬆️⬆️⬆️

    // 1. 状态维护 (State Check)
    switch (CurrentState)
    {
    case EUnitState::Idle:
        if (!CurrentTarget)
        {
            CurrentTarget = FindClosestTarget();
            if (CurrentTarget && GridManagerRef)
            {
                RequestPathToTarget();
                if (PathPoints.Num() > 0) CurrentState = EUnitState::Moving;
            }
        }
        else
        {
            ABaseGameEntity* TargetEntity = Cast<ABaseGameEntity>(CurrentTarget);
            if (!TargetEntity || TargetEntity->CurrentHealth <= 0) CurrentTarget = nullptr;
        }
        break;

    case EUnitState::Moving:
        if (CurrentTarget)
        {
            // --- 使用 GetClosestPointOnCollision ---
            float DistToSurface = FLT_MAX;
            UPrimitiveComponent* TargetPrim = Cast<UPrimitiveComponent>(CurrentTarget->GetRootComponent());
            if (!TargetPrim) TargetPrim = CurrentTarget->FindComponentByClass<UStaticMeshComponent>();

            if (TargetPrim)
            {
                FVector ClosestPoint;
                TargetPrim->GetClosestPointOnCollision(GetActorLocation(), ClosestPoint);
                ClosestPoint.Z = GetActorLocation().Z;
                DistToSurface = FVector::Dist(GetActorLocation(), ClosestPoint);
            }
            else
            {
                // 保底
                DistToSurface = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
            }

            // [进入门槛]：射程 + 10 (稍微宽容一点点，方便刹车)
            if (DistToSurface <= (AttackRange + 10.0f))
            {
                CurrentState = EUnitState::Attacking;
                PathPoints.Empty();
                // 可以在这里加个日志
                // UE_LOG(LogTemp, Log, TEXT("Enter Attack: Dist %f <= Range %f"), DistToSurface, AttackRange + 10.f);
            }
        }
        else
        {
            CurrentState = EUnitState::Idle;
        }
        break;

    case EUnitState::Attacking:
        PerformAttack();
        break;
    }

    // 2. 移动计算

    FVector FinalVelocity = FVector::ZeroVector;
    FVector CurrentLoc = GetActorLocation();

    // --- 力 A: 寻路/追击引力 ---
    if (CurrentState == EUnitState::Moving)
    {
        FVector MoveDir = FVector::ZeroVector;

        // 情况 1: 还有路径点，跟着 A* 走
        if (PathPoints.Num() > 0 && CurrentPathIndex < PathPoints.Num())
        {
            FVector TargetPoint = PathPoints[CurrentPathIndex];
            TargetPoint.Z = CurrentLoc.Z;
            MoveDir = (TargetPoint - CurrentLoc).GetSafeNormal();

            // 检查到达路点
            if (FVector::DistSquared2D(CurrentLoc, TargetPoint) < 900.0f)
            {
                CurrentPathIndex++;
            }
        }
        // 情况 2: [关键修复] 路径走完了/没路径，但还没打到人 -> 直奔目标！
        else if (CurrentTarget)
        {
            MoveDir = (CurrentTarget->GetActorLocation() - CurrentLoc).GetSafeNormal();
            MoveDir.Z = 0; // 锁死高度

            // 调试：画一条绿线，证明正在直追
            // DrawDebugLine(GetWorld(), CurrentLoc, CurrentTarget->GetActorLocation(), FColor::Green, false, -1, 0, 2.0f);
        }

        // 应用移动力
        FinalVelocity += MoveDir * MoveSpeed;
    }

    // --- 力 B: 强力避让 (Separation) ---
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bHit = GetWorld()->OverlapMultiByChannel(
        Overlaps, CurrentLoc, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(60.0f), Params
    );

    if (bHit)
    {
        FVector SeparationVec = FVector::ZeroVector;
        for (const FOverlapResult& Res : Overlaps)
        {
            ABaseUnit* OtherUnit = Cast<ABaseUnit>(Res.GetActor());
            // 推开所有活着的单位
            if (OtherUnit && OtherUnit->CurrentHealth > 0)
            {
                FVector Dir = CurrentLoc - OtherUnit->GetActorLocation();
                Dir.Z = 0;
                float Dist = Dir.Size();
                if (Dist < 60.0f)
                {
                    float PushStrength = (60.0f - Dist) / (Dist + 0.1f);
                    SeparationVec += Dir.GetSafeNormal() * PushStrength * 5000.0f;
                }
            }
        }
        FinalVelocity += SeparationVec;
    }
    

    if (CurrentState == EUnitState::Attacking)
    {
        // 减弱 90% 的推力，或者直接设为 ZeroVector
        FinalVelocity *= 0.1f;
    }

    // --- 3. 执行移动 ---
    if (!FinalVelocity.IsNearlyZero())
    {
        // 限制最大速度
        FinalVelocity = FinalVelocity.GetClampedToMaxSize(MoveSpeed * 2.0f);
        FinalVelocity.Z = 0.0f; // 绝对防钻地

        FHitResult MoveHit;
        AddActorWorldOffset(FinalVelocity * DeltaTime, true, &MoveHit);

        if (MoveHit.IsValidBlockingHit())
        {
            // 滑动处理
            FVector Normal = MoveHit.Normal;
            FVector SlideDir = FVector::VectorPlaneProject(FinalVelocity, Normal);
            AddActorWorldOffset(SlideDir * DeltaTime, true);
        }

        // 面向移动方向
        if (FinalVelocity.SizeSquared() > 100.0f)
        {
            FRotator TargetRot = FinalVelocity.Rotation();
            FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, 10.0f);
            SetActorRotation(NewRot);
        }
    }

    // --- 4. 攻击动画 (冲撞) ---
    if (bIsLunging && MeshComp)
    {
        LungeTimer += DeltaTime * 10.0f; // 动画速度
        float Alpha = FMath::Sin(LungeTimer);

        if (LungeTimer >= PI)
        {
            bIsLunging = false;
            LungeTimer = 0.0f;
            MeshComp->SetRelativeLocation(OriginalMeshOffset);
        }
        else
        {
            FVector ForwardOffset = FVector(Alpha * 150.0f, 0.f, 0.f); // 冲 150cm
            MeshComp->SetRelativeLocation(OriginalMeshOffset + ForwardOffset);
        }
    }
}

void ABaseUnit::SetUnitActive(bool bActive)
{
    bIsActive = bActive;
    CurrentState = EUnitState::Idle;
    if (!bActive)
    {
        CurrentTarget = nullptr;
        PathPoints.Empty();
    }
}

AActor* ABaseUnit::FindClosestTarget()
{
    AActor* ClosestActor = nullptr;
    float ClosestDistance = FLT_MAX;

    TArray<AActor*> AllEntities;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseGameEntity::StaticClass(), AllEntities);

    for (AActor* Actor : AllEntities)
    {
        ABaseGameEntity* Entity = Cast<ABaseGameEntity>(Actor);

        if (Entity &&
            Entity->TeamID != this->TeamID &&
            Entity->CurrentHealth > 0 &&
            Entity->bIsTargetable)
        {
            // 计算到表面的距离
            float DistToSurface = FLT_MAX;
            UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Entity->GetRootComponent());
            if (!Prim) Prim = Entity->FindComponentByClass<UStaticMeshComponent>();

            if (Prim)
            {
                FVector ClosestPt;
                Prim->GetClosestPointOnCollision(GetActorLocation(), ClosestPt);
                ClosestPt.Z = GetActorLocation().Z;
                DistToSurface = FVector::Dist(GetActorLocation(), ClosestPt);
            }
            else
            {
                DistToSurface = FVector::Dist(GetActorLocation(), Entity->GetActorLocation());
            }

            if (DistToSurface <= (AttackRange + 10.0f))
            {
                return Entity;
            }

            if (DistToSurface < ClosestDistance)
            {
                ClosestDistance = DistToSurface;
                ClosestActor = Entity;
            }
        }
    }

    return ClosestActor;
}

void ABaseUnit::RequestPathToTarget()
{
    if (!GridManagerRef)
    {
        GridManagerRef = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
    }

    if (!CurrentTarget || !GridManagerRef) return;

    FVector StartPos = GetActorLocation();
    FVector EndPos = CurrentTarget->GetActorLocation();

    // 查找路径
    PathPoints = GridManagerRef->FindPath(StartPos, EndPos);
    CurrentPathIndex = 0;

    // 路径抖动 (Jitter) - 防止重叠走线
    if (PathPoints.Num() > 0)
    {
        float JitterAmount = 40.0f;
        for (int32 i = 0; i < PathPoints.Num() - 1; i++)
        {
            PathPoints[i].X += FMath::RandRange(-JitterAmount, JitterAmount);
            PathPoints[i].Y += FMath::RandRange(-JitterAmount, JitterAmount);
        }
    }
}

void ABaseUnit::PerformAttack()
{
    // 更强的目标有效性检查
    if (!IsValid(CurrentTarget) || CurrentTarget->IsPendingKill())
    {
        CurrentTarget = nullptr;
        CurrentState = EUnitState::Idle;
        return;
    }

    // 转换为具体类型进行更严格的检查
    ABaseGameEntity* TargetEntity = Cast<ABaseGameEntity>(CurrentTarget);
    if (!TargetEntity || TargetEntity->CurrentHealth <= 0.0f || !TargetEntity->bIsTargetable)
    {
        CurrentTarget = nullptr;
        CurrentState = EUnitState::Idle;
        return;
    }

    if (!CurrentTarget)
    {
        CurrentState = EUnitState::Idle;
        return;
    }

    // 攻击时的距离检查也用表面距离
    float DistToSurface = FLT_MAX;
    UPrimitiveComponent* TargetPrim = Cast<UPrimitiveComponent>(CurrentTarget->GetRootComponent());
    if (!TargetPrim) TargetPrim = CurrentTarget->FindComponentByClass<UStaticMeshComponent>();

    if (TargetPrim)
    {
        FVector ClosestPt;
        TargetPrim->GetClosestPointOnCollision(GetActorLocation(), ClosestPt);
        ClosestPt.Z = GetActorLocation().Z;
        DistToSurface = FVector::Dist(GetActorLocation(), ClosestPt);
    }
    else
    {
        DistToSurface = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
    }

    // 宽松判定
    if (DistToSurface > (AttackRange + 80.0f))
    {
        RequestPathToTarget();
        if (PathPoints.Num() > 0) CurrentState = EUnitState::Moving;
        return;
    }

    // 攻击执行
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastAttackTime >= AttackInterval)
    {
        FDamageEvent DamageEvent;
        CurrentTarget->TakeDamage(Damage, DamageEvent, nullptr, this);
        LastAttackTime = CurrentTime;

        // 面向目标
        FVector Dir = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        if (!Dir.IsNearlyZero())
        {
            FRotator TargetRot = Dir.Rotation();
            TargetRot.Pitch = 0;
            SetActorRotation(TargetRot);
        }

        // 触发冲撞动画
        bIsLunging = true;
        LungeTimer = 0.0f;

        UE_LOG(LogTemp, Log, TEXT("[Unit] %s attacked %s!"), *GetName(), *CurrentTarget->GetName());
    }
}
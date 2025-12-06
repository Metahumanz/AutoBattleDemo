#include "RTSPlayerController.h"
#include "RTSGameMode.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"

ARTSPlayerController::ARTSPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    bIsPlacingUnit = false;
    PendingUnitType = EUnitType::Soldier;
}

void ARTSPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // 这里可以加一个 PlacementPreviewActor 跟随鼠标逻辑
}

void ARTSPlayerController::OnSelectUnitToPlace(EUnitType UnitType)
{
    PendingUnitType = UnitType;
    bIsPlacingUnit = true;
}

void ARTSPlayerController::HandleLeftClick()
{
    if (!bIsPlacingUnit) return;

    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    if (Hit.bBlockingHit)
    {
        ARTSGameMode* GM = Cast<ARTSGameMode>(GetWorld()->GetAuthGameMode());
        AGridManager* GridMgr = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));

        if (GM && GridMgr)
        {
            int32 X, Y;
            // 只有点在网格内才有效
            if (GridMgr->WorldToGrid(Hit.Location, X, Y))
            {
                if (GridMgr->IsTileWalkable(X, Y))
                {
                    // 这里价格先写死，或者通过配置表获取
                    int32 Cost = (PendingUnitType == EUnitType::Soldier) ? 50 : 100;

                    // 调用 GM 尝试购买
                    if (GM->TryBuyUnit(PendingUnitType, Cost, X, Y))
                    {
                        // 购买成功
                        // bIsPlacingUnit = false; // 可选：买完一个就取消，还是继续买
                    }
                }
            }
        }
    }
}

// 记得绑定输入
void ARTSPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction("LeftClick", IE_Pressed, this, &ARTSPlayerController::HandleLeftClick);
}
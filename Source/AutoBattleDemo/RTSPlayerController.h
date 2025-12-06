#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTSCoreTypes.h" // 引用枚举
#include "RTSPlayerController.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ARTSPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ARTSPlayerController();
    virtual void Tick(float DeltaTime) override;

    virtual void SetupInputComponent() override;

    // --- 交互逻辑 ---

    // 玩家点击了UI上的“购买步兵”
    UFUNCTION(BlueprintCallable)
        void OnSelectUnitToPlace(EUnitType UnitType);

    // 鼠标点击左键 (在 Blueprint 中绑定 Input Action: LeftClick)
    UFUNCTION(BlueprintCallable)
        void HandleLeftClick();

private:
    // 当前正在“拖拽/悬停”准备放置的单位类型
    EUnitType PendingUnitType;
    bool bIsPlacingUnit;

    // 一个临时的 Actor，跟着鼠标跑，显示预览效果
    UPROPERTY()
        AActor* PlacementPreviewActor;
};
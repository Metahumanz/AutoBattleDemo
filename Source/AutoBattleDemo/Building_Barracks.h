#pragma once
#include "CoreMinimal.h"
#include "BaseBuilding.h"
#include "Building_Barracks.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ABuilding_Barracks : public ABaseBuilding
{
    GENERATED_BODY()

public:
    ABuilding_Barracks();

    virtual void BeginPlay() override;

    // 关键：当物体被销毁（被移除或被打爆）时调用
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
    // 提供的额外人口上限
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
        int32 PopulationBonus;
};
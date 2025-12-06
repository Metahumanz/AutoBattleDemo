#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RTSCoreTypes.h"
#include "RTSGameMode.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ARTSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARTSGameMode();
	virtual void BeginPlay() override;

	// --- 流程控制 API (供 UI 调用) ---

	// 1. 尝试购买并放置单位 (注意：这里补全了 GridX 和 GridY 参数)
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
		bool TryBuyUnit(EUnitType Type, int32 Cost, int32 GridX, int32 GridY);

	// 2. 玩家点击“开始战斗”
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
		void StartBattlePhase();

	// 3. 重新开始本关
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
		void RestartLevel();

	// --- 裁判逻辑 ---

	// 4. 单位死亡时调用此函数 (这就是你刚才报错缺少的函数声明！)
	void OnActorKilled(AActor* Victim, AActor* Killer);

	// 5. 检查是否胜利
	void CheckWinCondition();

protected:
	// 当前游戏状态
	UPROPERTY(BlueprintReadOnly, Category = "GameFlow")
		EGameState CurrentState;

	// 引用：地图上的 GridManager
	UPROPERTY()
		class AGridManager* GridManager;

	// 配置：不同兵种的蓝图类 (用于 Spawn)
	UPROPERTY(EditDefaultsOnly, Category = "Classes")
		TSubclassOf<class ABaseUnit> SoldierClass;

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
		TSubclassOf<class ABaseUnit> ArcherClass;
};
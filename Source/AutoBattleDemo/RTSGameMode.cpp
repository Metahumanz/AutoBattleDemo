#include "RTSGameMode.h"
#include "RTSPlayerController.h"
#include "GridManager.h"
#include "BaseUnit.h"
#include "RTSGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

ARTSGameMode::ARTSGameMode()
{
	// 设置默认控制器
	PlayerControllerClass = ARTSPlayerController::StaticClass();
	CurrentState = EGameState::Preparation;
}

void ARTSGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 缓存 GridManager
	GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
}

// 核心逻辑：买兵
bool ARTSGameMode::TryBuyUnit(EUnitType Type, int32 Cost, int32 GridX, int32 GridY)
{
	// 只有备战阶段能买
	if (CurrentState != EGameState::Preparation) return false;

	// 1. 检查金币
	URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
	// 如果没有 GameInstance，默认给个 9999 钱方便测试，防止一直买不了
	int32 CurrentGold = GI ? GI->PlayerGold : 9999;

	if (CurrentGold < Cost)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not enough gold!"));
		return false;
	}

	// 2. 选择生成的蓝图类
	TSubclassOf<ABaseUnit> SpawnClass = nullptr;
	if (Type == EUnitType::Soldier) SpawnClass = SoldierClass;
	else if (Type == EUnitType::Archer) SpawnClass = ArcherClass;

	// 如果蓝图没配，或者 GridManager 没放，就失败
	if (!SpawnClass || !GridManager)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnClass not set in BP_RTSGameMode or GridManager missing!"));
		return false;
	}

	// 3. 扣钱
	if (GI) GI->PlayerGold -= Cost;

	// 4. 计算世界坐标
	FVector SpawnLoc = GridManager->GridToWorld(GridX, GridY);
	SpawnLoc.Z += 100.0f; // 稍微抬高防止卡住

	// 5. 生成单位
	ABaseUnit* NewUnit = GetWorld()->SpawnActor<ABaseUnit>(SpawnClass, SpawnLoc, FRotator::ZeroRotator);
	if (NewUnit)
	{
		NewUnit->TeamID = ETeam::Player; // 标记为玩家阵营

		// 告诉 GridManager 这里有人了
		GridManager->SetTileBlocked(GridX, GridY, true);
		return true;
	}

	return false;
}

void ARTSGameMode::StartBattlePhase()
{
	CurrentState = EGameState::Battle;

	// 遍历所有单位，告诉它们“开打了”
	for (TActorIterator<ABaseUnit> It(GetWorld()); It; ++It)
	{
		if (*It) (*It)->SetUnitActive(true);
	}

	UE_LOG(LogTemp, Log, TEXT("Battle Started!"));
}

void ARTSGameMode::RestartLevel()
{
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), false);
}

// 这就是你报错缺少的函数实现
void ARTSGameMode::OnActorKilled(AActor* Victim, AActor* Killer)
{
	// 可以在这里写胜负逻辑
	UE_LOG(LogTemp, Warning, TEXT("Actor Killed: %s"), *Victim->GetName());

	CheckWinCondition();
}

void ARTSGameMode::CheckWinCondition()
{
	// 简单的示例：检查是否还有敌人
	// 实际逻辑由你根据需求扩充
}
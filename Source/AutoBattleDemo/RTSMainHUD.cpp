#include "RTSMainHUD.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "RTSPlayerController.h"
#include "RTSGameMode.h"
#include "RTSGameInstance.h"

void URTSMainHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// 绑定单位按钮
	if (Btn_BuyBarbarian)  Btn_BuyBarbarian->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuyBarbarian);
	if (Btn_BuyArcher)  Btn_BuyArcher->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuyArcher);
	if (Btn_BuyGiant)   Btn_BuyGiant->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuyGiant);
	if (Btn_BuyBomber)  Btn_BuyBomber->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuyBomber);

	// 绑定建筑按钮
	if (Btn_BuildTower) Btn_BuildTower->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuildTower);
	if (Btn_BuildMine)  Btn_BuildMine->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuildMine);
	if (Btn_BuildElixir) Btn_BuildElixir->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuildElixir);
	if (Btn_BuildWall)	Btn_BuildWall->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuildWall);
	if (Btn_BuildBarracks)	Btn_BuildBarracks->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuildBarracks);

	// 绑定流程按钮
	if (Btn_StartBattle) Btn_StartBattle->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickStartBattle);
	
	// 绑定移除按钮
	if (Btn_Remove) Btn_Remove->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickRemove);
}

void URTSMainHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
	if (GI)
	{
		if (Text_GoldInfo)   Text_GoldInfo->SetText(FText::FromString(FString::Printf(TEXT("Gold: %d"), GI->PlayerGold)));
		if (Text_ElixirInfo) Text_ElixirInfo->SetText(FText::FromString(FString::Printf(TEXT("Elixir: %d"), GI->PlayerElixir)));
		
		// 更新人口
		if (Text_PopulationInfo)
		{
			// 格式示例： Pop: 5 / 20
			FString PopStr = FString::Printf(TEXT("Pop: %d / %d"),
				GI->CurrentPopulation,
				GI->MaxPopulation);

			Text_PopulationInfo->SetText(FText::FromString(PopStr));
		}
	}
}

// 辅助函数：处理点击并归还焦点
void URTSMainHUD::OnClickBuyBarbarian()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		PC->OnSelectUnitToPlace(EUnitType::Barbarian);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuyArcher()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		PC->OnSelectUnitToPlace(EUnitType::Archer);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuyGiant()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		PC->OnSelectUnitToPlace(EUnitType::Giant);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuyBomber()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		PC->OnSelectUnitToPlace(EUnitType::Bomber);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuildTower()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		PC->OnSelectBuildingToPlace(EBuildingType::Defense); // 注意：对应 GameMode switch 里的类型
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuildMine()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		PC->OnSelectBuildingToPlace(EBuildingType::GoldMine);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuildElixir()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		// 告诉 Controller 我要造圣水收集器
		PC->OnSelectBuildingToPlace(EBuildingType::ElixirPump);

		// 归还鼠标焦点
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

// 实现造墙逻辑
void URTSMainHUD::OnClickBuildWall()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC)
	{
		// 告诉 Controller：我要造墙
		PC->OnSelectBuildingToPlace(EBuildingType::Wall);

		// 归还鼠标焦点 (老规矩，防止点完按钮后没法点地板)
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

// 实现造兵营逻辑
void URTSMainHUD::OnClickBuildBarracks()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC)
	{
		PC->OnSelectBuildingToPlace(EBuildingType::Barracks);

		// 归还鼠标焦点
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

void URTSMainHUD::OnClickStartBattle()
{
	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		// 假设战斗地图叫 "BattleField1"
		GM->SaveAndStartBattle(FName("BattleField1"));
	}
}

void URTSMainHUD::OnClickRemove()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC)
	{
		PC->OnSelectRemoveMode();
		// 归还焦点
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}
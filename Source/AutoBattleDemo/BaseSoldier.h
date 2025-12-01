// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseSoldier.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ABaseSoldier : public ACharacter
{
	GENERATED_BODY()

public:
	// 构造函数声明
	ABaseSoldier();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// --- 核心属性 ---

	// 攻击力 (EditAnywhere 允许在蓝图里改为 10 或 50)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
		float Damage;

	// 攻击距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
		float AttackRange;

	// 攻击间隔 (攻速)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
		float AttackInterval;

	// --- 核心多态函数 ---

	// 虚函数：发起攻击
	// 加上 'virtual' 关键字，意味着子类(Archer)可以覆盖它
	virtual void Attack(AActor* Target);
};
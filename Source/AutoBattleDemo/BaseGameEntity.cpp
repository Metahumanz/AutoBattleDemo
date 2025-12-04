#include "BaseGameEntity.h"
#include "RTSGameMode.h"
#include "Kismet/GameplayStatics.h"

ABaseGameEntity::ABaseGameEntity()
{
    PrimaryActorTick.bCanEverTick = true;

    // 默认值
    MaxHealth = 100.0f;
    CurrentHealth = MaxHealth;
    TeamID = ETeam::Enemy; // 默认为敌人，子类可修改
}

float ABaseGameEntity::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    CurrentHealth -= ActualDamage;
    // UE_LOG(LogTemp, Warning, TEXT("%s took %f damage. Current HP: %f"), *GetName(), ActualDamage, CurrentHealth);

    if (CurrentHealth <= 0.0f)
    {
        Die();
    }

    return ActualDamage;
}

void ABaseGameEntity::Die()
{
    // 通知 GameMode (我是受害者，谁杀了我这里暂时传空)
    ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
    if (GM)
    {
        GM->OnActorKilled(this, nullptr);
    }

    // 销毁自己
    Destroy();
}
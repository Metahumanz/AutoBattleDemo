#include "Soldier_Barbarian.h"

ASoldier_Barbarian::ASoldier_Barbarian()
{
    // 野蛮人属性：高血量，近战
    MaxHealth = 150.0f;
    AttackRange = 150.0f;  // 近战距离
    Damage = 15.0f;
    MoveSpeed = 250.0f;     // 较慢
    AttackInterval = 1.0f;
}

void ASoldier_Barbarian::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[Barbarian] %s spawned | HP: %f | Damage: %f"),
        *GetName(), MaxHealth, Damage);
}
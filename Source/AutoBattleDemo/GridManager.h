#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSCoreTypes.h"
#include "GridManager.generated.h"

// 定义格子的结构体
USTRUCT(BlueprintType)
struct FGridNode
{
    GENERATED_BODY()

        // 格子坐标
        int32 X;
    int32 Y;

    // 世界坐标中心点
    FVector WorldLocation;

    // 是否被阻挡（有墙或防御塔）
    bool bIsBlocked;

    // 寻路消耗 (可以预留，比如沼泽地走得慢)
    float Cost;
};

UCLASS()
class AUTOBATTLEDEMO_API AGridManager : public AActor
{
    GENERATED_BODY()

public:
    AGridManager();

    // --- 供成员 C 调用：初始化地图 ---
    // 生成可视化格子（Debug线或模型），初始化 GridArray
    UFUNCTION(BlueprintCallable, Category = "Grid")
        void GenerateGrid(int32 Width, int32 Height, float CellSize);

    // --- 供成员 C 调用：交互转换 ---
    // 鼠标点的世界坐标 -> 格子坐标 (用于造兵时吸附网格)
    UFUNCTION(BlueprintCallable, Category = "Grid")
        bool WorldToGrid(FVector WorldPos, int32& OutX, int32& OutY);

    // 格子坐标 -> 世界坐标 (用于把兵摆在格子正中间)
    UFUNCTION(BlueprintCallable, Category = "Grid")
        FVector GridToWorld(int32 X, int32 Y);

    // --- 供成员 C 调用：阻挡设置 ---
    // 检查是否可以放置 (不能重叠，也不能放在障碍物上)
    UFUNCTION(BlueprintCallable, Category = "Grid")
        bool IsTileWalkable(int32 X, int32 Y);

    // 放置建筑后调用，锁定该格子
    UFUNCTION(BlueprintCallable, Category = "Grid")
        void SetTileBlocked(int32 X, int32 Y, bool bBlocked);

    // --- 供成员 B 调用：核心寻路 ---
    // 输入：起点世界坐标，终点世界坐标
    // 输出：路径点列表 (World Locations)
    // 重点：如果找不到路径，返回空数组
    UFUNCTION(BlueprintCallable, Category = "Pathfinding")
        TArray<FVector> FindPath(FVector StartPos, FVector EndPos);

public:
    // 二维数组扁平化存储，或者使用 TArray<FGridNode>
    // 建议使用 TMap<FIntPoint, FGridNode> 或者一维数组 Index = Y * Width + X
    TArray<FGridNode> GridNodes;

    UPROPERTY(EditAnywhere, Category = "Grid Config")
        int32 GridWidthCount;

    UPROPERTY(EditAnywhere, Category = "Grid Config")
        int32 GridHeightCount;

    UPROPERTY(EditAnywhere, Category = "Grid Config")
        float TileSize;
};
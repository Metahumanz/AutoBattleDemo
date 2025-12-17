#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridManager.generated.h"

USTRUCT(BlueprintType)
struct FGridNode
{
    GENERATED_BODY()

        UPROPERTY()
        int32 X;
    UPROPERTY()
        int32 Y;
    UPROPERTY()
        bool bIsBlocked;
    UPROPERTY()
        FVector WorldLocation;
    UPROPERTY()
        float Cost;
};

UCLASS()
class AUTOBATTLEDEMO_API AGridManager : public AActor
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

public:
    AGridManager();

    UFUNCTION(BlueprintCallable, Category = "Grid")
        bool IsTileWalkable(int32 X, int32 Y);

    UFUNCTION(BlueprintCallable, Category = "Grid")
        void GenerateGrid(int32 Width, int32 Height, float CellSize);

    UFUNCTION(BlueprintCallable, Category = "Grid")
        TArray<FVector> FindPath(const FVector& StartWorldLoc, const FVector& EndWorldLoc);

    UFUNCTION(BlueprintCallable, Category = "Grid")
        void SetTileBlocked(int32 GridX, int32 GridY, bool bBlocked);

    UFUNCTION(BlueprintCallable, Category = "Grid")
        FVector GridToWorld(int32 GridX, int32 GridY) const;

    UFUNCTION(BlueprintCallable, Category = "Grid")
        bool WorldToGrid(const FVector& WorldLoc, int32& OutGridX, int32& OutGridY) const;

    UFUNCTION(BlueprintCallable, Category = "Grid")
        void DrawGridVisuals(int32 HoverX, int32 HoverY);

private:
    // A*算法节点（纯内部使用，无需USTRUCT）
    struct FAStarNode
    {
        int32 X;
        int32 Y;
        float G; // 起点到当前节点成本
        float H; // 预估到终点成本
        FAStarNode* Parent;

        // 计算总成本
        float F() const { return G + H; }
        FAStarNode(int32 InX, int32 InY) : X(InX), Y(InY), G(0), H(0), Parent(nullptr) {}
    };

    // 工具函数：从TArray中找到F值最小的节点（模拟优先级队列）
    FAStarNode* GetLowestFNode(TArray<FAStarNode*>& Nodes);

    // 核心辅助函数
    bool IsTileValid(int32 GridX, int32 GridY) const;
    float GetHeuristicCost(int32 X1, int32 Y1, int32 X2, int32 Y2) const;
    TArray<FIntPoint> GetNeighborNodes(int32 X, int32 Y) const;
    void OptimizePath(TArray<FIntPoint>& RawPath);

    // 网格数据
    UPROPERTY()
        TArray<FGridNode> GridNodes;
    UPROPERTY()
        int32 GridWidthCount;
    UPROPERTY()
        int32 GridHeightCount;
    UPROPERTY()
        float TileSize;
    UPROPERTY(EditAnywhere, Category = "Debug")
        bool bDrawDebug;
};
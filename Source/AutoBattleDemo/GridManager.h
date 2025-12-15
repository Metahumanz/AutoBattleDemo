// GridManager.h（接口保持不变，仅补充必要声明）
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridManager.generated.h"

/**
 * 网格节点结构体，存储单个格子的所有数据
 */
USTRUCT(BlueprintType)
struct FGridNode
{
    GENERATED_BODY()

        // 格子在网格中的X坐标
        UPROPERTY()
        int32 X;
    // 格子在网格中的Y坐标
    UPROPERTY()
        int32 Y;
    // 是否被阻挡（如建筑、障碍物）
    UPROPERTY()
        bool bIsBlocked;
    // 格子中心点的世界坐标
    UPROPERTY()
        FVector WorldLocation;
    // 地形成本（影响移动消耗，平地1.0，沼泽等可设更高值）
    UPROPERTY()
        float Cost;
};

/**
 * 网格管理器类，负责网格生成、坐标转换和路径查找
 */
UCLASS()
class AUTOBATTLEDEMO_API AGridManager : public AActor
{
    GENERATED_BODY()

protected:
    // --- 声明游戏开始函数 ---
    virtual void BeginPlay() override;

public:
    AGridManager();
    UFUNCTION(BlueprintCallable, Category = "Grid")
        bool IsTileWalkable(int32 X, int32 Y);
    /**
     * 生成网格并初始化所有节点
     * @param Width 网格宽度（X方向格子数量）
     * @param Height 网格高度（Y方向格子数量）
     * @param CellSize 每个格子的尺寸（世界单位）
     */
    UFUNCTION(BlueprintCallable, Category = "Grid")
        void GenerateGrid(int32 Width, int32 Height, float CellSize);

    /**
     * 查找从起点到终点的路径（A*算法实现）
     * @param StartWorldLoc 起点世界坐标
     * @param EndWorldLoc 终点世界坐标
     * @return 路径点列表（世界坐标），若找不到路径则返回空数组
     */
    UFUNCTION(BlueprintCallable, Category = "Grid")
        TArray<FVector> FindPath(const FVector& StartWorldLoc, const FVector& EndWorldLoc);

    /**
     * 设置指定格子的阻挡状态
     * @param GridX 格子X坐标
     * @param GridY 格子Y坐标
     * @param bBlocked 是否阻挡
     */
    UFUNCTION(BlueprintCallable, Category = "Grid")
        void SetTileBlocked(int32 GridX, int32 GridY, bool bBlocked);

    /**
     * 将网格坐标转换为世界坐标
     * @param GridX 格子X坐标
     * @param GridY 格子Y坐标
     * @return 对应格子中心点的世界坐标
     */
    UFUNCTION(BlueprintCallable, Category = "Grid")
        FVector GridToWorld(int32 GridX, int32 GridY) const;

    /**
     * 将世界坐标转换为网格坐标
     * @param WorldLoc 世界坐标
     * @param OutGridX 输出格子X坐标
     * @param OutGridY 输出格子Y坐标
     * @return 是否成功转换（坐标在网格范围内）
     */
    UFUNCTION(BlueprintCallable, Category = "Grid")
        bool WorldToGrid(const FVector& WorldLoc, int32& OutGridX, int32& OutGridY) const;

    // 每帧调用的绘制函数 ---
    // HoverX, HoverY: 当前鼠标悬停的格子坐标 (如果没有悬停传 -1)
    UFUNCTION(BlueprintCallable, Category = "Grid")
        void DrawGridVisuals(int32 HoverX, int32 HoverY);

private:
    /**
     * A*算法节点结构体，用于路径计算
     */
    struct FAStarNode
    {
        int32 X;               // 节点X坐标
        int32 Y;               // 节点Y坐标
        float G;               // 起点到当前节点的实际成本
        float H;               // 当前节点到终点的预估成本（启发式）
        TWeakPtr<FAStarNode> Parent;  // 父节点（用于回溯路径）

        // 计算总成本（F = G + H）
        float F() const { return G + H; }
        // 构造函数
        FAStarNode(int32 InX, int32 InY) : X(InX), Y(InY), G(0), H(0) {}
    };

    /**
     * 检查格子是否有效（在网格范围内且未被阻挡）
     * @param GridX 格子X坐标
     * @param GridY 格子Y坐标
     * @return 是否有效
     */
    bool IsTileValid(int32 GridX, int32 GridY) const;

    /**
     * 计算启发式成本（曼哈顿距离）
     * @param X1 起点X
     * @param Y1 起点Y
     * @param X2 终点X
     * @param Y2 终点Y
     * @return 启发式成本值
     */
    float GetHeuristicCost(int32 X1, int32 Y1, int32 X2, int32 Y2) const;

    /**
     * 获取指定格子的所有有效邻居节点（四方向）
     * @param X 格子X坐标
     * @param Y 格子Y坐标
     * @return 邻居节点坐标列表
     */
    TArray<FIntPoint> GetNeighborNodes(int32 X, int32 Y) const;

    /**
     * 优化路径（移除冗余节点，使路径更平滑）
     * @param RawPath 原始路径
     */
    void OptimizePath(TArray<FIntPoint>& RawPath);

    // 存储所有网格节点（一维数组模拟二维）
    UPROPERTY()
        TArray<FGridNode> GridNodes;
    // 网格宽度（X方向格子数量）
    UPROPERTY()
        int32 GridWidthCount;
    // 网格高度（Y方向格子数量）
    UPROPERTY()
        int32 GridHeightCount;
    // 每个格子的尺寸（世界单位）
    UPROPERTY()
        float TileSize;
    // 调试绘制开关（开发模式使用）
    UPROPERTY(EditAnywhere, Category = "Debug")
        bool bDrawDebug;


};

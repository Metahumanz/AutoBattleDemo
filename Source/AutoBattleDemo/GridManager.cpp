#include "GridManager.h"
#include "DrawDebugHelpers.h" // 用于画线调试

// 构造函数
AGridManager::AGridManager()
{
    PrimaryActorTick.bCanEverTick = false; // 不需要每帧运行

    // 默认配置
    GridWidthCount = 20;
    GridHeightCount = 20;
    TileSize = 100.0f;
}

// 生成网格数据
void AGridManager::GenerateGrid(int32 Width, int32 Height, float CellSize)
{
    GridWidthCount = Width;
    GridHeightCount = Height;
    TileSize = CellSize;

    GridNodes.Empty();

    // 简单的双重循环初始化
    for (int32 Y = 0; Y < Height; Y++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            FGridNode NewNode;
            NewNode.X = X;
            NewNode.Y = Y;
            NewNode.bIsBlocked = false;

            // 计算中心点世界坐标 (假设 GridManager 放在 0,0,0)
            // 如果 GridManager 移动了，这里要加上 GetActorLocation()
            NewNode.WorldLocation = GetActorLocation() + FVector(X * TileSize + TileSize / 2, Y * TileSize + TileSize / 2, 0);

            GridNodes.Add(NewNode);

            // 画线调试 (画一个方框)
            DrawDebugBox(GetWorld(), NewNode.WorldLocation, FVector(TileSize / 2 * 0.9f, TileSize / 2 * 0.9f, 5.0f), FColor::White, true, -1.0f);
        }
    }
}

// 世界坐标 -> 格子坐标
bool AGridManager::WorldToGrid(FVector WorldPos, int32& OutX, int32& OutY)
{
    // 相对坐标
    FVector LocalPos = WorldPos - GetActorLocation();

    OutX = FMath::FloorToInt(LocalPos.X / TileSize);
    OutY = FMath::FloorToInt(LocalPos.Y / TileSize);

    // 边界检查
    if (OutX >= 0 && OutX < GridWidthCount && OutY >= 0 && OutY < GridHeightCount)
    {
        return true;
    }
    return false;
}

// 格子坐标 -> 世界坐标
FVector AGridManager::GridToWorld(int32 X, int32 Y)
{
    // 简单的数学公式：起点 + 偏移 + 半个格子大小
    float WorldX = (X * TileSize) + (TileSize / 2.0f);
    float WorldY = (Y * TileSize) + (TileSize / 2.0f);

    return GetActorLocation() + FVector(WorldX, WorldY, 0.0f);
}

// 检查是否可走
bool AGridManager::IsTileWalkable(int32 X, int32 Y)
{
    // 1. 边界检查
    if (X < 0 || X >= GridWidthCount || Y < 0 || Y >= GridHeightCount)
    {
        return false;
    }

    // 2. 算出数组索引 (一维数组模拟二维)
    int32 Index = Y * GridWidthCount + X;
    if (GridNodes.IsValidIndex(Index))
    {
        return !GridNodes[Index].bIsBlocked;
    }

    return false;
}

// 设置阻挡
void AGridManager::SetTileBlocked(int32 X, int32 Y, bool bBlocked)
{
    int32 Index = Y * GridWidthCount + X;
    if (GridNodes.IsValidIndex(Index))
    {
        GridNodes[Index].bIsBlocked = bBlocked;

        // 视觉反馈：变成红色
        if (bBlocked)
        {
            FVector Center = GridToWorld(X, Y);
            DrawDebugBox(GetWorld(), Center, FVector(TileSize / 2, TileSize / 2, 10.0f), FColor::Red, true, -1.0f);
        }
    }
}

// 核心寻路 (目前给个空实现，为了让 Member B 不报错)
TArray<FVector> AGridManager::FindPath(FVector StartPos, FVector EndPos)
{
    TArray<FVector> Path;

    // 暂时先返回直连，防止游戏崩溃
    // 等 Member A 写好了 A* 算法再替换这里
    Path.Add(StartPos);
    Path.Add(EndPos);

    return Path;
}
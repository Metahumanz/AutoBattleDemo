#include "GridManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Misc/AssertionMacros.h"

AGridManager::AGridManager()
{
    PrimaryActorTick.bCanEverTick = false;
    bDrawDebug = true;

    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

void AGridManager::BeginPlay()
{
    Super::BeginPlay();
    GenerateGrid(20, 20, 100.0f);
}

void AGridManager::GenerateGrid(int32 Width, int32 Height, float CellSize)
{
    GridWidthCount = Width;
    GridHeightCount = Height;
    TileSize = CellSize;
    GridNodes.Empty();
    GridNodes.Reserve(Width * Height);

    for (int32 Y = 0; Y < Height; Y++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            FGridNode NewNode;
            NewNode.X = X;
            NewNode.Y = Y;
            NewNode.bIsBlocked = false;
            NewNode.Cost = 1.0f;
            NewNode.WorldLocation = GetActorLocation() + FVector(
                X * TileSize + TileSize / 2,
                Y * TileSize + TileSize / 2,
                0.0f
            );
            GridNodes.Add(NewNode);
        }
    }
}

void AGridManager::DrawGridVisuals(int32 HoverX, int32 HoverY)
{
    if (!bDrawDebug) return;

    float LifeTime = GetWorld()->GetDeltaSeconds() * 2.0f;

    for (const FGridNode& Node : GridNodes)
    {
        FColor LineColor = FColor(110, 110, 110);
        float LineThickness = 5.0f;

        if (Node.X == HoverX && Node.Y == HoverY)
        {
            LineColor = FColor::Cyan;
            LineThickness = 10.0f;
        }
        else if (Node.bIsBlocked)
        {
            LineColor = FColor::Red;
            LineThickness = 7.5f;
        }

        DrawDebugBox(
            GetWorld(),
            Node.WorldLocation,
            FVector(TileSize / 2 * 0.90f, TileSize / 2 * 0.90f, 5.0f),
            LineColor,
            false,
            LifeTime,
            0,
            LineThickness
        );
    }
}

// 核心：模拟优先级队列 - 找到F值最小的节点
AGridManager::FAStarNode* AGridManager::GetLowestFNode(TArray<FAStarNode*>& Nodes)
{
    if (Nodes.IsEmpty()) return nullptr;

    FAStarNode* LowestNode = Nodes[0];
    for (FAStarNode* Node : Nodes)
    {
        if (Node->F() < LowestNode->F())
        {
            LowestNode = Node;
        }
    }
    return LowestNode;
}

// 修正后的FindPath（无TPriorityQueue依赖）
TArray<FVector> AGridManager::FindPath(const FVector& StartWorldLoc, const FVector& EndWorldLoc)
{
    TArray<FVector> Path;
    int32 StartX, StartY, EndX, EndY;

    // 坐标转换（非静态调用）
    if (!WorldToGrid(StartWorldLoc, StartX, StartY) || !WorldToGrid(EndWorldLoc, EndX, EndY))
    {
        UE_LOG(LogTemp, Warning, TEXT("Start/End out of grid bounds"));
        return Path;
    }

    // 终点不可行走则返回空
    if (!IsTileWalkable(EndX, EndY))
    {
        UE_LOG(LogTemp, Warning, TEXT("End tile is blocked"));
        return Path;
    }

    // 初始化OpenList（TArray替代优先级队列）、ClosedList、节点映射
    TArray<FAStarNode*> OpenList;
    TSet<FIntPoint> ClosedList;
    TMap<FIntPoint, FAStarNode*> NodeMap;

    // 创建起点节点
    FAStarNode* StartNode = new FAStarNode(StartX, StartY);
    StartNode->H = GetHeuristicCost(StartX, StartY, EndX, EndY);
    OpenList.Add(StartNode);
    NodeMap.Add(FIntPoint(StartX, StartY), StartNode);

    while (!OpenList.IsEmpty())
    {
        // 取F值最小的节点（模拟优先级队列出队）
        FAStarNode* CurrentNode = GetLowestFNode(OpenList);
        OpenList.Remove(CurrentNode); // 从OpenList移除
        FIntPoint CurrentPos(CurrentNode->X, CurrentNode->Y);

        // 已处理过则跳过
        if (ClosedList.Contains(CurrentPos))
        {
            delete CurrentNode; // 避免内存泄漏
            continue;
        }
        ClosedList.Add(CurrentPos);

        // 到达终点：回溯路径
        if (CurrentNode->X == EndX && CurrentNode->Y == EndY)
        {
            TArray<FIntPoint> RawPath;
            FAStarNode* TempNode = CurrentNode;
            while (TempNode != nullptr)
            {
                RawPath.Insert(FIntPoint(TempNode->X, TempNode->Y), 0);
                TempNode = TempNode->Parent;
            }

            // 路径优化 + 转换为世界坐标
            OptimizePath(RawPath);
            for (const FIntPoint& Point : RawPath)
            {
                Path.Add(GridToWorld(Point.X, Point.Y));
            }

            // 清理所有动态分配的节点
            for (auto& Pair : NodeMap) delete Pair.Value;
            return Path;
        }

        // 处理邻居节点
        TArray<FIntPoint> Neighbors = GetNeighborNodes(CurrentNode->X, CurrentNode->Y);
        for (const FIntPoint& NeighborPos : Neighbors)
        {
            if (ClosedList.Contains(NeighborPos)) continue;

            // 计算移动成本（格子成本 * 距离）
            float MoveCost = FVector::Dist(
                GridToWorld(CurrentNode->X, CurrentNode->Y),
                GridToWorld(NeighborPos.X, NeighborPos.Y)
            ) * GridNodes[NeighborPos.Y * GridWidthCount + NeighborPos.X].Cost;

            float NewGCost = CurrentNode->G + MoveCost;
            FAStarNode* NeighborNode = nullptr;

            // 节点已存在则复用，否则新建
            if (NodeMap.Contains(NeighborPos))
            {
                NeighborNode = NodeMap[NeighborPos];
                if (NewGCost >= NeighborNode->G) continue; // 成本更高则跳过
            }
            else
            {
                NeighborNode = new FAStarNode(NeighborPos.X, NeighborPos.Y);
                NodeMap.Add(NeighborPos, NeighborNode);
            }

            // 更新节点信息
            NeighborNode->G = NewGCost;
            NeighborNode->H = GetHeuristicCost(NeighborPos.X, NeighborPos.Y, EndX, EndY);
            NeighborNode->Parent = CurrentNode;

            // 加入OpenList（未存在则添加）
            if (!OpenList.Contains(NeighborNode))
            {
                OpenList.Add(NeighborNode);
            }
        }
    }

    // 未找到路径：清理内存
    for (auto& Pair : NodeMap) delete Pair.Value;
    UE_LOG(LogTemp, Warning, TEXT("No path found"));
    return Path;
}

// 其余函数（SetTileBlocked/GridToWorld/WorlToGrid等）保持不变
void AGridManager::SetTileBlocked(int32 GridX, int32 GridY, bool bBlocked)
{
    if (!IsTileValid(GridX, GridY)) return;

    int32 Index = GridY * GridWidthCount + GridX;
    GridNodes[Index].bIsBlocked = bBlocked;

    if (bDrawDebug)
    {
        DrawDebugBox(
            GetWorld(),
            GridNodes[Index].WorldLocation,
            FVector(TileSize / 2 * 0.9f, TileSize / 2 * 0.9f, 2.0f),
            bBlocked ? FColor::Red : FColor::White,
            true,
            30.0f,
            0,
            3.0f
        );
    }
}

FVector AGridManager::GridToWorld(int32 GridX, int32 GridY) const
{
    if (!IsTileValid(GridX, GridY)) return FVector::ZeroVector;
    return GridNodes[GridY * GridWidthCount + GridX].WorldLocation;
}

bool AGridManager::WorldToGrid(const FVector& WorldLoc, int32& OutGridX, int32& OutGridY) const
{
    FVector LocalLoc = WorldLoc - GetActorLocation();
    OutGridX = FMath::FloorToInt(LocalLoc.X / TileSize);
    OutGridY = FMath::FloorToInt(LocalLoc.Y / TileSize);
    return IsTileValid(OutGridX, OutGridY);
}

bool AGridManager::IsTileValid(int32 GridX, int32 GridY) const
{
    return GridX >= 0 && GridX < GridWidthCount&& GridY >= 0 && GridY < GridHeightCount;
}

bool AGridManager::IsTileWalkable(int32 X, int32 Y)
{
    if (!IsTileValid(X, Y)) return false;
    int32 Index = Y * GridWidthCount + X;
    return GridNodes.IsValidIndex(Index) && !GridNodes[Index].bIsBlocked;
}

float AGridManager::GetHeuristicCost(int32 X1, int32 Y1, int32 X2, int32 Y2) const
{
    // 曼哈顿距离（适合四方向移动）
    return FMath::Abs(X1 - X2) + FMath::Abs(Y1 - Y2);
}

TArray<FIntPoint> AGridManager::GetNeighborNodes(int32 X, int32 Y) const
{
    TArray<FIntPoint> Neighbors;
    // 四方向（上下左右）
    const int32 Directions[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

    for (const auto& Dir : Directions)
    {
        int32 NewX = X + Dir[0];
        int32 NewY = Y + Dir[1];
        // 仅添加有效且可行走的邻居
        if (IsTileValid(NewX, NewY) && !GridNodes[NewY * GridWidthCount + NewX].bIsBlocked)
        {
            Neighbors.Add(FIntPoint(NewX, NewY));
        }
    }
    return Neighbors;
}

void AGridManager::OptimizePath(TArray<FIntPoint>& RawPath)
{
    if (RawPath.Num() <= 2) return;

    TArray<FIntPoint> Optimized;
    Optimized.Add(RawPath[0]);
    FIntPoint PrevDir = RawPath[1] - RawPath[0];

    for (int32 i = 2; i < RawPath.Num(); i++)
    {
        FIntPoint CurrentDir = RawPath[i] - RawPath[i - 1];
        // 方向变化时保留节点
        if (CurrentDir != PrevDir)
        {
            Optimized.Add(RawPath[i - 1]);
            PrevDir = CurrentDir;
        }
    }
    Optimized.Add(RawPath.Last());
    RawPath = Optimized;
}
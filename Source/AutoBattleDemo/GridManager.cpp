// GridManager.cpp（核心实现改进）
#include "GridManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Containers/Queue.h"
#include "Misc/AssertionMacros.h"


/**
 * 构造函数：初始化默认值
 */
AGridManager::AGridManager()
{
    PrimaryActorTick.bCanEverTick = false;  // 不需要每帧更新
    bDrawDebug = true;                      // 默认开启调试绘制（开发模式）

    // 创建一个根组件，否则它在场景里没有坐标（Location 全是 0）
    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

/**
 * 游戏开始时调用
 */
void AGridManager::BeginPlay()
{
    Super::BeginPlay();

    GenerateGrid(20, 20, 100.0f);




}

/**
 * 生成网格并初始化所有节点
 * @param Width 网格宽度（X方向格子数量）
 * @param Height 网格高度（Y方向格子数量）
 * @param CellSize 每个格子的尺寸（世界单位）
 */
void AGridManager::GenerateGrid(int32 Width, int32 Height, float CellSize)
{
    GridWidthCount = Width;
    GridHeightCount = Height;
    TileSize = CellSize;
    GridNodes.Empty();
    GridNodes.Reserve(Width * Height);  // 预分配内存，减少动态扩容开销

    // 双重循环初始化所有格子
    for (int32 Y = 0; Y < Height; Y++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            FGridNode NewNode;
            NewNode.X = X;
            NewNode.Y = Y;
            NewNode.bIsBlocked = false;       // 默认所有格子可通行
            NewNode.Cost = 1.0f;              // 默认地形成本为1.0
            // 计算格子中心点世界坐标（基于管理器自身位置）
            NewNode.WorldLocation = GetActorLocation() + FVector(
                X * TileSize + TileSize / 2,  // X方向偏移（加一半尺寸居中）
                Y * TileSize + TileSize / 2,  // Y方向偏移（加一半尺寸居中）
                0.0f                         // Z轴默认为0（忽略高度）
            );
            GridNodes.Add(NewNode);

            //// 调试绘制格子边框（仅在非编辑器世界且开启调试时）
            //if (bDrawDebug && GetWorld()->GetName() != TEXT("EditorWorld"))
            //{
            //    DrawDebugBox(
            //        GetWorld(),
            //        NewNode.WorldLocation,
            //        FVector(TileSize / 2 * 0.9f, TileSize / 2 * 0.9f, 1.0f),  // 稍微缩小一点避免边框重叠
            //        FColor::White,
            //        true,                           // 持续显示
            //        -1.0f,                          // 永久存在
            //        0,
            //        4.0f                            // 线宽
            //    );
            //}
        }
    }


}

void AGridManager::DrawGridVisuals(int32 HoverX, int32 HoverY)
{
    float LifeTime = GetWorld()->GetDeltaSeconds() * 2.0f;

    for (const FGridNode& Node : GridNodes)
    {
        // 默认状态 (模拟半透明)
        // 使用灰色代替白色
        FColor LineColor = FColor(110, 110, 110);

        // 默认线宽变细，让它退居背景
        float LineThickness = 5.0f;

        // 1. 选中状态 (高亮 + 加粗)
        if (Node.X == HoverX && Node.Y == HoverY)
        {
            LineColor = FColor::Cyan; // 青色比蓝色更显眼
            LineThickness = 10.0f;     // 选中时很粗
        }
        // 2. 阻挡状态 (红色 + 中粗)
        else if (Node.bIsBlocked)
        {
            LineColor = FColor::Red;
            LineThickness = 7.5f;
        }

        // 绘制
        DrawDebugBox(
            GetWorld(),
            Node.WorldLocation,
            // 普通格子稍微小一点(0.9f)，给线之间留缝隙，看着更舒服
            // 选中格子可以稍微大一点吗？这里统一点比较好
            FVector(TileSize / 2 * 0.90f, TileSize / 2 * 0.90f, 5.0f),
            LineColor,
            false,
            LifeTime,
            0,
            LineThickness
        );
    }
}

/**
 * 查找从起点到终点的路径（A*算法实现）
 * @param StartWorldLoc 起点世界坐标
 * @param EndWorldLoc 终点世界坐标
 * @return 路径点列表（世界坐标）
 */
TArray<FVector> AGridManager::FindPath(const FVector& StartWorldLoc, const FVector& EndWorldLoc)
{
    TArray<FVector> Path;  // 最终路径（世界坐标）
    int32 StartX, StartY, EndX, EndY;

    // 1. 将起点和终点世界坐标转换为网格坐标并校验
    if (!WorldToGrid(StartWorldLoc, StartX, StartY) || !WorldToGrid(EndWorldLoc, EndX, EndY))
    {
        UE_LOG(LogTemp, Warning, TEXT("Start or end position is out of grid bounds"));
        return Path;  // 坐标超出网格范围，返回空路径
    }
    if (!IsTileValid(StartX, StartY) || !IsTileValid(EndX, EndY))
    {
        UE_LOG(LogTemp, Warning, TEXT("Start or end tile is blocked"));
        return Path;  // 起点或终点被阻挡，返回空路径
    }

    // 2. A*算法核心实现
    // 开放集：待检查的节点（用Map存储便于查找）
    TMap<FIntPoint, TSharedPtr<FAStarNode>> OpenSet;
    // 关闭集：已检查的节点
    TMap<FIntPoint, TSharedPtr<FAStarNode>> ClosedSet;
    // 初始化起点节点
    auto StartNode = MakeShareable(new FAStarNode(StartX, StartY));
    OpenSet.Add(FIntPoint(StartX, StartY), StartNode);

    // 循环处理开放集中的节点
    while (OpenSet.Num() > 0)
    {
        // 找到开放集中F值最小的节点（总成本最低）
        TSharedPtr<FAStarNode> CurrentNode = nullptr;
        FIntPoint CurrentKey;
        for (const auto& Pair : OpenSet)
        {
            if (!CurrentNode || Pair.Value->F() < CurrentNode->F())
            {
                CurrentNode = Pair.Value;
                CurrentKey = Pair.Key;
            }
        }

        // 到达终点，回溯路径
        if (CurrentKey.X == EndX && CurrentKey.Y == EndY)
        {
            TArray<FIntPoint> RawPath;  // 原始路径（网格坐标）
            // 从终点回溯到起点
            while (CurrentNode.IsValid())
            {
                RawPath.Add(FIntPoint(CurrentNode->X, CurrentNode->Y));
                CurrentNode = CurrentNode->Parent.Pin();  // 沿父节点回溯
            }
            // 替换 Algo::Reverse(RawPath); 为手动反转
            int32 PathLength = RawPath.Num();
            for (int32 i = 0; i < PathLength / 2; ++i)
            {
                // 交换第i个和第(PathLength-1-i)个元素
                FIntPoint Temp = RawPath[i];
                RawPath[i] = RawPath[PathLength - 1 - i];
                RawPath[PathLength - 1 - i] = Temp;
            }
            OptimizePath(RawPath);  // 优化路径（移除冗余点）

            // 将网格坐标转换为世界坐标
            for (const auto& GridPos : RawPath)
            {
                Path.Add(GridToWorld(GridPos.X, GridPos.Y));
            }
            return Path;
        }

        // 将当前节点从开放集移到关闭集
        OpenSet.Remove(CurrentKey);
        ClosedSet.Add(CurrentKey, CurrentNode);

        // 处理当前节点的所有邻居
        for (const auto& Neighbor : GetNeighborNodes(CurrentNode->X, CurrentNode->Y))
        {
            FIntPoint NeighborKey = Neighbor;
            // 跳过已在关闭集或不可通行的节点
            if (ClosedSet.Contains(NeighborKey) || !IsTileValid(Neighbor.X, Neighbor.Y))
            {
                continue;
            }

            // 计算从起点到当前邻居的实际成本
            float NewG = CurrentNode->G + GetHeuristicCost(
                CurrentNode->X, CurrentNode->Y,
                Neighbor.X, Neighbor.Y
            ) * GridNodes[Neighbor.Y * GridWidthCount + Neighbor.X].Cost;

            TSharedPtr<FAStarNode> NeighborNode;
            // 如果邻居已在开放集
            if (OpenSet.Contains(NeighborKey))
            {
                NeighborNode = OpenSet[NeighborKey];
                // 新路径成本更高，无需更新
                if (NewG >= NeighborNode->G) continue;
            }
            // 邻居不在开放集，创建新节点
            else
            {
                NeighborNode = MakeShareable(new FAStarNode(Neighbor.X, Neighbor.Y));
                OpenSet.Add(NeighborKey, NeighborNode);
            }

            // 更新邻居节点信息
            NeighborNode->G = NewG;  // 更新实际成本
            NeighborNode->H = GetHeuristicCost(Neighbor.X, Neighbor.Y, EndX, EndY);  // 更新预估成本
            NeighborNode->Parent = CurrentNode;  // 设置父节点
        }
    }

    // 开放集为空且未找到终点，寻路失败
    UE_LOG(LogTemp, Warning, TEXT("No path found between start and end"));
    return Path;
}

/**
 * 设置指定格子的阻挡状态
 * @param GridX 格子X坐标
 * @param GridY 格子Y坐标
 * @param bBlocked 是否阻挡
 */
void AGridManager::SetTileBlocked(int32 GridX, int32 GridY, bool bBlocked)
{
    // 检查格子是否有效
    if (!IsTileValid(GridX, GridY)) return;

    // 更新阻挡状态
    int32 Index = GridY * GridWidthCount + GridX;
    GridNodes[Index].bIsBlocked = bBlocked;

    // 调试显示：阻挡的格子显示红色边框
    if (bDrawDebug)
    {
        DrawDebugBox(
            GetWorld(),
            GridNodes[Index].WorldLocation,
            FVector(TileSize / 2 * 0.9f, TileSize / 2 * 0.9f, 2.0f),
            bBlocked ? FColor::Red : FColor::White,  // 阻挡为红色，否则白色
            true,
            30.0f,  // 持续30秒（便于观察）
            0,
            3.0f    // 线宽加粗
        );
    }
}

/**
 * 将网格坐标转换为世界坐标
 * @param GridX 格子X坐标
 * @param GridY 格子Y坐标
 * @return 对应格子中心点的世界坐标
 */
FVector AGridManager::GridToWorld(int32 GridX, int32 GridY) const
{
    // 检查格子是否有效
    if (!IsTileValid(GridX, GridY)) return FVector::ZeroVector;

    // 返回预计算的世界坐标
    return GridNodes[GridY * GridWidthCount + GridX].WorldLocation;
}

/**
 * 将世界坐标转换为网格坐标
 * @param WorldLoc 世界坐标
 * @param OutGridX 输出格子X坐标
 * @param OutGridY 输出格子Y坐标
 * @return 是否成功转换（坐标在网格范围内）
 */
bool AGridManager::WorldToGrid(const FVector& WorldLoc, int32& OutGridX, int32& OutGridY) const
{
    // 转换为相对于网格管理器的本地坐标
    FVector LocalLoc = WorldLoc - GetActorLocation();

    // 计算网格坐标（向下取整）
    OutGridX = FMath::FloorToInt(LocalLoc.X / TileSize);
    OutGridY = FMath::FloorToInt(LocalLoc.Y / TileSize);

    // 检查是否在网格范围内
    return IsTileValid(OutGridX, OutGridY);
}

/**
 * 检查格子是否有效（在网格范围内且未被阻挡）
 * @param GridX 格子X坐标
 * @param GridY 格子Y坐标
 * @return 是否有效
 */
bool AGridManager::IsTileValid(int32 GridX, int32 GridY) const
{
    // 检查坐标是否在网格范围内
    if (GridX < 0 || GridX >= GridWidthCount || GridY < 0 || GridY >= GridHeightCount)
        return false;

    // 检查格子是否未被阻挡
    return !GridNodes[GridY * GridWidthCount + GridX].bIsBlocked;
}

bool AGridManager::IsTileWalkable(int32 X, int32 Y)
{
    // 1. 检查坐标是否超出网格范围（越界则不可走）
    if (X < 0 || X >= GridWidthCount || Y < 0 || Y >= GridHeightCount)
    {
        return false;
    }

    // 2. 计算一维数组索引（将二维坐标转换为扁平化数组索引）
    int32 Index = Y * GridWidthCount + X;

    // 3. 检查索引有效性并返回是否可走（!bIsBlocked 表示可走）
    if (GridNodes.IsValidIndex(Index))
    {
        return !GridNodes[Index].bIsBlocked;
    }

    // 索引无效时默认不可走
    return false;
}
/**
 * 计算启发式成本（曼哈顿距离）
 * 曼哈顿距离：|x1-x2| + |y1-y2|，适用于四方向移动
 * @param X1 起点X
 * @param Y1 起点Y
 * @param X2 终点X
 * @param Y2 终点Y
 * @return 启发式成本值
 */
float AGridManager::GetHeuristicCost(int32 X1, int32 Y1, int32 X2, int32 Y2) const
{
    return FMath::Abs(X1 - X2) + FMath::Abs(Y1 - Y2);
}

/**
 * 获取指定格子的所有有效邻居节点（四方向：上下左右）
 * @param X 格子X坐标
 * @param Y 格子Y坐标
 * @return 邻居节点坐标列表
 */
TArray<FIntPoint> AGridManager::GetNeighborNodes(int32 X, int32 Y) const
{
    TArray<FIntPoint> Neighbors;
    // 四方向偏移量（右、左、上、下）
    const int32 Directions[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

    // 检查每个方向的邻居是否有效
    for (const auto& Dir : Directions)
    {
        int32 NewX = X + Dir[0];
        int32 NewY = Y + Dir[1];
        if (IsTileValid(NewX, NewY))
        {
            Neighbors.Add(FIntPoint(NewX, NewY));
        }
    }
    return Neighbors;
}

/**
 * 优化路径（移除冗余节点，使路径更平滑）
 * 原理：当连续三个点在同一直线上时，中间点可省略
 * @param RawPath 原始路径
 */
void AGridManager::OptimizePath(TArray<FIntPoint>& RawPath)
{
    // 路径点太少无需优化
    if (RawPath.Num() <= 2) return;

    TArray<FIntPoint> Optimized;
    Optimized.Add(RawPath[0]);  // 添加起点
    FIntPoint PrevDir = RawPath[1] - RawPath[0];  // 初始方向

    // 遍历原始路径，保留方向改变的点
    for (int32 i = 2; i < RawPath.Num(); i++)
    {
        FIntPoint CurrentDir = RawPath[i] - RawPath[i - 1];
        // 方向改变时，保留前一个点
        if (CurrentDir != PrevDir)
        {
            Optimized.Add(RawPath[i - 1]);
            PrevDir = CurrentDir;
        }
    }
    Optimized.Add(RawPath.Last());  // 添加终点
    RawPath = Optimized;  // 更新为优化后的路径
}
# SchedulerPluginDev · 架构DSL

> 更新：2026-06-11
> 用途：项目长久记忆，记录功能所有者、广播、接口与委托链
> 维护规则：每次增删委托/接口/函数签名/UI控件后同步更新本文

---

## 插件类型

| 属性 | 值 |
|------|-----|
| 类型 | **Runtime**（非 Editor-only） |
| 版本 | 1.1.1（Version 编码 10101 = MAJOR×10000 + MINOR×100 + PATCH） |
| 依赖模块 | Core, CoreUObject, Engine, Slate, SlateCore, UMG, InputCore |
| 打包 | ✅ UnrealEditor + UnrealGame 双目标通过 |

---

## 源码目录结构

```
Plugins/Scheduler/Source/Scheduler/
├── Public/
│   ├── Scheduler.h                    [模块入口]
│   ├── Core/
│   │   └── SchedulerSubsystem.h       [内核]
│   ├── Task/
│   │   ├── SchedulerTask.h            [任务数据]
│   │   └── TaskInterface.h            [接口层]
│   └── UI/
│       ├── USchedulerWidget.h         [UMG总容器]
│       ├── SSchedulerWidget.h         [Slate十字布局]
│       ├── Playhead/
│       │   └── SSchedulerPlayhead.h   [游标控件]
│       ├── Ruler/
│       │   ├── USchedulerRuler.h      [UMG刻度尺]
│       │   ├── SSchedulerRuler.h      [Slate刻度尺]
│       │   └── SchedulerRulerTypes.h  [刻度类型定义]
│       └── Track/
│           ├── SSchedulerTrackTitle.h [行标题控件]
│           └── SSchedulerTrackBody.h  [行体控件]
└── Private/
    ├── Scheduler.cpp
    ├── Core/
    │   └── SchedulerSubsystem.cpp
    ├── Task/
    │   └── SchedulerTask.cpp
    └── UI/
        ├── USchedulerWidget.cpp
        ├── SSchedulerWidget.cpp
        ├── Playhead/
        │   └── SSchedulerPlayhead.cpp
        ├── Ruler/
        │   ├── USchedulerRuler.cpp
        │   └── SSchedulerRuler.cpp
        └── Track/
            ├── SSchedulerTrackTitle.cpp
            └── SSchedulerTrackBody.cpp
```

---

## 模块树

```
Scheduler (插件模块)
├── USchedulerSubsystem : UWorldSubsystem   [内核——时刻调度中心 + Task工厂/池 + Track池]
├── USchedulerTask : UObject                [任务数据单元——关键帧 + 时刻回调]
│   └── FClipIndex : USTRUCT               [关键帧区间——LastIndex/NextIndex]
├── ITaskInterface / UTaskInterface         [接口层——Task通知Owner的唯一回调]
├── USchedulerWidget : UWidget              [UI层——总容器，蓝图唯一暴露口]
│   └── SSchedulerWidget : SCompoundWidget  [Slate——十字分割布局]
├── USchedulerRuler : UWidget               [UI层——水平刻度尺，支持滚动/缩放/点击]
│   └── SSchedulerRuler : SCompoundWidget   [Slate——刻度绘制+输入处理]
├── SSchedulerPlayhead : SCompoundWidget    [UI层——游标竖线 + 时间标签]
├── SSchedulerTrackTitle : SCompoundWidget  [Track行——BodyLeft单行标题(箭头+文字+删除)]
└── SSchedulerTrackBody : SCompoundWidget   [Track行——BodyRight单行体(Keyframe渲染)]
```

---

## 接口层

### ITaskInterface —— Task → Owner 通知接口

**定位：** 替代原有的 `FExecuteTask` 多播委托。Task 对 Owner 是唯一的，接口回调语义更精确。蓝图可直接实现。

**文件：** `Public/Task/TaskInterface.h`

```
UINTERFACE(Blueprintable)
class UTaskInterface : public UInterface

class ITaskInterface
{
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void ExecuteTask(double Alpha, bool bIsForward, FClipIndex ClipIndex);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void DestroyTask();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void RemoveKeyframe(int32 KeyframeIndex);
};
```

**调用方式（C++）：**

```
ITaskInterface::Execute_ExecuteTask(TaskOwner, Alpha, bIsForward, ClipIndex);
ITaskInterface::Execute_RemoveKeyframe(TaskOwner, Index);
```

`Execute_ExecuteTask` 是 UHT 为 BlueprintNativeEvent 生成的静态分派函数，第一参数为目标对象。

---

## 任务层

### FClipIndex —— 关键帧区间

**定位：** 描述当前时刻在 Keyframes 数组中的位置区间。

| 属性 | 类型 | 默认值 | 说明 |
|------|------|------|------|
| `LastIndex` | int32 | -1 | 最后一个 ≤ CurrentTime 的关键帧索引 |
| `NextIndex` | int32 | -1 | 第一个 ≥ CurrentTime 的关键帧索引 |

**方向语义（Keyframes = [0, 5, 8, 20]）：**

| CurrentTime | bIsForward | LastIndex | NextIndex | 含义 |
|-------------|------------|-----------|-----------|------|
| 8 | true | 2 | 3 | 命中8，下一帧是20 |
| 20 | true | 3 | 3 | 命中末帧，无下一帧 |
| 8 | false | 1 | 2 | 命中8，上一帧是5 |
| 0 | false | 0 | 0 | 命中首帧，无上一帧 |
| 3 | 任意 | 0 | 1 | 落在0和5之间 |

**计算逻辑：** 二分查找定位 Left（首个 ≥ CurrentTime 的索引），精确命中时按方向偏移 Last/Next，落在区间内则 [Left-1, Left]。

---

### USchedulerTask —— 任务数据单元

**定位：** 单个调度任务的 UObject 封装。携带关键帧数组，通过 Subsystem 绑定时刻变更回调，在 `OnTimeChange` 中二分查找并通知 Owner。

| 属性 | 值 |
|------|-----|
| 父类 | UObject |
| 文件 | `Public/Task/SchedulerTask.h` |

**成员变量：**

| 变量 | 类型 | 说明 |
|------|------|------|
| `Keyframes` | `TArray<int64>` | 关键帧时间点数组，有序 |
| `TaskName` | `FString` | 任务名称 |
| `TaskOwnerName` | `FString` | 拥有者名称（TaskMap 的分组键） |
| `TaskOwner` | `UObject*` | 拥有者指针（即 Outer，实现 ITaskInterface） |
| `bLeftBoundaryReported` | `bool` | 左边界已报告标记，连续超出同侧仅触发一次 |
| `bRightBoundaryReported` | `bool` | 右边界已报告标记，进入一侧时清除对侧 |

**函数：**

| 函数 | 访问 | 类别 | 功能 |
|------|------|------|------|
| `OnTaskInitialized()` | public | — | 初始化钩子，预留 TaskTrack::Create 调用 |
| `OnDestroy()` | public | — | 销毁钩子，预留 UI 销毁逻辑 |
| `OnTimeChange(int64, bool)` | public | — | 时刻变更回调：二分查找 → 构造 ClipIndex → 计算区间进度 Alpha → ITaskInterface::Execute_ExecuteTask |
| `AddKeyframe(int64, const TArray<int64>&, int32&, bool&)` | public, BlueprintCallable | Scheduler\|Task | 拷贝 InKeyframes 到成员 Keyframes，二分查找插入/去重，触发 RefreshAllKeyframes |
| `RemoveKeyframe(int64, int32&)` | public | — | 按值查找并移除关键帧，返回原始索引，通知 TaskOwner RemoveKeyframe，触发 RefreshAllKeyframes |
| `SyncKeyframes(const TArray<int64>&)` | public, BlueprintCallable | Scheduler\|Task | Save/Load 后数据恢复入口——替换 Keyframes、排序去重、触发 UI 重绘 |

**OnTimeChange 流程：**

```
空数组检查
  ├─ 超出左边界（InCurrentTime < Keyframes[0]）
  │   ├─ bLeftBoundaryReported == true → 静默 return（防连续重复广播）
  │   ├─ 置 bLeftBoundaryReported = true, bRightBoundaryReported = false
  │   ├─ 构造 ClipIndex {0, 0}，Alpha = 0.0
  │   └─ if !IsValid(TaskOwner) return → ITaskInterface::Execute_ExecuteTask
  │
  ├─ 超出右边界（InCurrentTime > Keyframes.Last()）
  │   ├─ bRightBoundaryReported == true → 静默 return
  │   ├─ 置 bRightBoundaryReported = true, bLeftBoundaryReported = false
  │   ├─ 构造 ClipIndex {LastIdx, LastIdx}，Alpha = 1.0
  │   └─ if !IsValid(TaskOwner) return → ITaskInterface::Execute_ExecuteTask
  │
  └─ 区间内
      ├─ 重置 bLeftBoundaryReported = bRightBoundaryReported = false
      ├─ 二分查找：Left = 首个 >= InCurrentTime 的索引
      ├─ 构造 ClipIndex（精确命中按方向，区间内统一 [Left-1, Left]）
      ├─ 计算 Alpha = (InCurrentTime - Keyframes[LastIndex]) / (Keyframes[NextIndex] - Keyframes[LastIndex])
      │   边界（LastIndex == NextIndex）钳位为 0.0，防除零
      └─ if !IsValid(TaskOwner) return → ITaskInterface::Execute_ExecuteTask
```

**边界去重规则：** 进入一侧边界时清除对侧标记——交替穿越（左→右→左）可连续触发。连续停留在同侧则仅广播首次。回到区间内后两标记均重置。

**SyncKeyframes 流程（Save/Load 后数据恢复入口）：**

```
Keyframes = InKeyframes       —— 用户外部维护的数据接管进来
  → Sort()                    —— 二分查找前置条件
  → 倒序去重                   —— 重复值破坏 ClipIndex 语义
  → Subsystem->RefreshAllKeyframes() —— UI 面片重绘
```

---

## 内核层

### USchedulerSubsystem —— 时刻调度中心 + Task 工厂/池 + Track 池

**定位：** 内核变量 `CurrentTime` 完全私有；Task 的创建、管理池、Track 控件创建、OnTimeChanged 绑定统一收敛于此。

**文件：** `Public/Core/SchedulerSubsystem.h`

### FOnTimeChanged

| 属性 | 值 |
|------|-----|
| 声明宏 | `DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams` |
| 参数1 | `int64 NewCurrentTime` — 变更后的新时刻 |
| 参数2 | `bool bIsForward` — true=前进，false=后退 |
| 持有者 | `USchedulerSubsystem::OnTimeChanged` |
| Blueprint | `BlueprintAssignable` |

**广播源（完整调用链）：**

```
initializationSubsystem()
    └→ OnTimeChanged.Broadcast(0, true)

SetCurrentTime(NewCurrentTime)
    ├─ 门禁：NewCurrentTime == CurrentTime → return
    ├─ 门禁：NewCurrentTime < 0 → return
    ├─ bIsForward = (NewCurrentTime == 0) || (NewCurrentTime > CurrentTime) — 归零强制为正向
    └→ OnTimeChanged.Broadcast(CurrentTime, bIsForward)

CurrentTimePlusPlus()
    └→ OnTimeChanged.Broadcast(CurrentTime, true)

CurrentTimeMinusMinus()
    ├─ 触底：CurrentTime <= 0 → 静默 return
    ├─ --CurrentTime
    ├─ bIsForward = (CurrentTime == 0) — 归零正向，与 SetCurrentTime(0) 对齐
    └→ OnTimeChanged.Broadcast(CurrentTime, bIsForward)
```

**函数清单：**

| 函数 | 访问 | 类别 | 功能 | 副作用 |
|------|------|------|------|------|
| `initializationSubsystem()` | public, BlueprintCallable | Scheduler | 重置 CurrentTime=0，广播 | 触发 OnTimeChanged |
| `GetCurrentTime() const` | public, BlueprintPure | Scheduler\|TimeChange | 返回当前时刻 | 无 |
| `SetCurrentTime(int64)` | public, BlueprintCallable | Scheduler\|TimeChange | 跳转到指定时刻；归零时强制 bIsForward=true | 触发 OnTimeChanged（有门禁） |
| `CurrentTimePlusPlus()` | public, BlueprintCallable | Scheduler\|TimeChange | 前进1时刻 | 触发 OnTimeChanged |
| `CurrentTimeMinusMinus()` | public, BlueprintCallable | Scheduler\|TimeChange | 后退1时刻；触底(0)静默，归零时正向广播 | 条件触发 OnTimeChanged |
| `CreateTask(FString, FString, UObject*)` | public, BlueprintCallable | Scheduler\|Task | 工厂创建 Task 并入池 | 绑定 OnTimeChanged |
| `DestroyTask(USchedulerTask*)` | public, BlueprintCallable | Scheduler\|Task | 从 TaskMap/TrackMap 移除 Task + UI，清理空分组 | 触发 OnDestroy + GC |
| `DestroyOwner(FString)` | public, BlueprintCallable | Scheduler\|Task | 批量销毁 Owner 下全部 Task + Track | 触发全部子 Task 的 OnDestroy + 清理分组 |
| `InitializeTrackContainers(...)` | public | — | Widget 初始化时注入 ScrollBox/SVerticalBox 引用 + 全部显示属性 | 补齐已有 TrackMap 中缺失控件 |
| `SyncKeyframeState(...)` | public | — | 同步 Ruler 状态 + Keyframe 渲染参数到缓存 | 供 RefreshAllKeyframes 读取 |
| `RefreshAllKeyframes()` | public | — | 遍历所有 TaskTrack → Body->RefreshKeyframes() | 重绘全部 Keyframe 面片 |

**CreateTask 流程：**

```
NewObject<USchedulerTask>(TaskOwner)
  → 赋值 TaskName, TaskOwnerName, TaskOwner
  → OnTaskInitialized()
  → TaskMap.FindOrAdd(TaskOwnerName).Add(NewTask)
  → OnTimeChanged.AddDynamic(NewTask, &USchedulerTask::OnTimeChange)
  → return NewTask
```

**DestroyTask 流程：**

```
IsValid(Task) 门禁
  → TrackMap.Find(TaskOwnerName) + TaskMap.Find(TaskOwnerName)
  → 匹配 FTaskTrackEntry.Task → DestroyTaskTrackWidgets → RemoveSlot
  → OwnerTasks->Remove(Task)
  → 分组为空 → DestroyOwnerTrackWidgets → TrackMap.Remove + TaskMap.Remove
  → Task->OnDestroy() → 解绑 OnTimeChanged + ITaskInterface::Execute_DestroyTask + MarkAsGarbage
  → return true
```

**DestroyOwner 流程（批量销毁，不循环调 DestroyTask 以避免 TaskMap 键中途失效）：**

```
TaskMap.Find(OwnerName) 门禁——无则 return false
  → TrackMap.Find(OwnerName)——可能为 nullptr（UI 未初始化）
  → for Task in *OwnerTasks:
       IsValid(Task) → Task->OnDestroy()（bIsOnDestroy 防重入）
  → if OwnerTrack: DestroyOwnerTrackWidgets（内部遍历子 TaskTrack + RemoveSlot）
  → TrackMap.Remove(OwnerName)
  → TaskMap.Remove(OwnerName)——延后移除，确保 OnDestroy 期间数据完整
  → return true
```

**TaskMap / TrackMap：**

| 属性 | 类型 | 说明 |
|------|------|------|
| `TaskMap` | `TMap<FString, TArray<USchedulerTask*>>` | 按 OwnerName 分组管理所有 Task |
| `TrackMap` | `TMap<FString, FTrack>` | UI 池，与 TaskMap 键镜像，持有 Title/Body + 子 TaskTrack 列表 |

---

## 订阅关系图（完整委托链）

```
[外部调用]
    │
    ├─ C++: GetWorld()->GetSubsystem<USchedulerSubsystem>()
    ├─ Blueprint: Get Subsystem (WorldSubsystem)
    │
    ▼
USchedulerSubsystem
    │
    ├── CreateTask(TaskOwner) ─→ NewObject → TaskMap 入池 → AddDynamic 绑定
    ├── GetCurrentTime()          ← 只读，无副作用
    ├── SetCurrentTime(t)         ← 门禁校验 → 修改 → Broadcast
    ├── CurrentTimePlusPlus()     ← ++ → Broadcast
    ├── CurrentTimeMinusMinus()   ← -- → 钳位 → 条件Broadcast
    └── initializationSubsystem() ← 归零 → Broadcast
              │
              ▼
    FOnTimeChanged.Broadcast(NewCurrentTime, bIsForward)
              │
              ▼
    USchedulerTask::OnTimeChange(Time, Dir)
              │
              ├─ 二分查找 Keyframes，构造 FClipIndex
              │
              ▼
    ITaskInterface::Execute_ExecuteTask(TaskOwner, Alpha, Dir, ClipIndex)
              │
              ▼
    TaskOwner (实现 ITaskInterface 的蓝图/C++对象)
```

---

## UI 层

### SchedulerWidget —— 插件 UI 总容器

**定位：** 纯布局容器，自身无任何交互功能与渲染刷新（无 Tick）。四个 Slot 为后续 UI 子项提供挂载点，所有功能由 Slot 内的子项实现。

**分层架构：**

| 层 | 类 | 文件 |
|------|------|------|
| UMG 包装（蓝图可见） | `USchedulerWidget` | `Public/UI/USchedulerWidget.h` |
| Slate 实现 | `SSchedulerWidget` | `Public/UI/SSchedulerWidget.h` |

**十字分割布局：**

```
┌──────────────────────────────────────────┐
│  HeadLeft  Slot  │  HeadRight  Slot      │ ← Head 区，高度 = HeadHeight（像素）
├──────────────────┼───────────────────────┤ ← 水平分割线（bIsDrawCross 控制）
│  BodyLeft  Slot  │  BodyRight  Slot      │ ← Body 区，填充剩余空间
└──────────────────┴───────────────────────┘
       ↑                          ↑
  垂直分割线               右区填充剩余宽度
  左区宽度 = LeftSidebarWidth（像素）
```

**蓝图属性（Layout）：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|------|------|
| `HeadHeight` | float | 100 | Head 区固定高度（像素） |
| `LeftSidebarWidth` | float | 200 | 左区固定宽度（像素），≥0 |
| `bIsDrawCross` | bool | true | 是否绘制十字分割线 |

**蓝图属性（Ruler）：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|------|------|
| `RulerStyle` | `FSchedulerRulerStyle` | — | 刻度线颜色、粗细、背景色 |
| `RulerMajorLength` | float | 15 | 大刻度线像素高度 |
| `RulerMinorLength` | float | 5 | 小刻度线像素高度 |
| `RulerMinorPixel` | float | 10 | 每 Minor 格子固定像素宽度 |
| `RulerTickOffset` | float | 0 | 刻度起始 Y 偏移 |
| `RulerMaxTickValue` | int64 | 0 | 刻度轴最大 Tick 值，0=不限制 |
| `RulerTickLevel` | `TArray<FTickLevel>` | [] | 缩放层级数组 |

**蓝图属性（Slot）：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|------|------|
| `Customize` | `TSubclassOf<UUserWidget>` | nullptr | 左上角自定义控件类——用户选取 UUserWidget 子类，运行时动态创建实例 |

**事件（UMG）：**

| 委托 | 说明 |
|------|------|
| `RefreshUI` | UI 同步刷新信号 |

**四个 Slot：**

| Slot 名 | 位置 | 填充方式 |
|------|------|------|
| `HeadLeft` | 左上 | 蓝图 `Customize` 属性（UUserWidget 实例），或 C++ `.HeadLeft(...)` |
| `HeadRight` | 右上 | `.HeadRight(MyRulerSlate)` — SSchedulerRuler |
| `BodyLeft` | 左下 | `.BodyLeft(TitleScrollBox)` — 垂直滚动列表，容纳 SSchedulerTrackTitle 行 |
| `BodyRight` | 右下 | `.BodyRight(BodyScrollBox)` — 垂直滚动列表，容纳 SSchedulerTrackBody 行 |

> HeadRight 上方叠加 `SSchedulerPlayhead`（SOverlay 覆盖整个 Widget），竖线贯穿 HeadRight + BodyRight。

**生命周期约束：**

| 约束 | 说明 |
|------|------|
| 无 Tick | `SSchedulerWidget` 不覆写 `Tick` |
| 按需重绘 | `bIsDrawCross` 变更时 Invalidate |
| 按需重排 | `HeadHeight`/`LeftSidebarWidth` 变更时 InvalidateLayout |
| 子项自治 | Slot 中填充的 Widget 自行管理 |
| Slate 释放 | `USchedulerWidget::ReleaseSlateResources` 已覆写 |

---

### SchedulerRuler —— 水平刻度尺

**定位：** 水平 Tick 刻度尺，支持左键点击/拖拽设时刻、右键拖拽滚动、滚轮缩放。

**分层架构：**

| 层 | 类 | 文件 |
|------|------|------|
| UMG 包装 | `USchedulerRuler` | `Public/UI/Ruler/USchedulerRuler.h` |
| Slate 实现 | `SSchedulerRuler` | `Public/UI/Ruler/SSchedulerRuler.h` |
| 类型定义 | `FTickLevel` / `FSchedulerRulerStyle` | `Public/UI/Ruler/SchedulerRulerTypes.h` |

**成员变量：**

| 变量 | 类型 | 默认值 | 说明 |
|------|------|------|------|
| `Style` | `FSchedulerRulerStyle` | — | 刻度线颜色、粗细、背景色、字号 |
| `MajorLength` | float | 15 | 大刻度线像素高度 |
| `MinorLength` | float | 5 | 小刻度线像素高度 |
| `MinorPixel` | float | 10 | 每 Minor 格子固定像素宽度 |
| `TickOffset` | float | 0 | 刻度起始 Y 偏移 |
| `MaxTickValue` | int64 | 0 | 刻度轴最大 Tick 值，0=不限制 |
| `ActiveLevelIndex` | int32 | 0 | 当前 TickLevel 索引 |
| `ViewStartTick` | int64 | 0 | 可见区域起始 Tick |
| `TickLevel` | `TArray<FTickLevel>` | [] | 缩放层级数组 |

**FTickLevel：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|------|------|
| `Minor` | int64 | 1 | 1 Minor 刻度 = ? Tick |
| `Major` | int64 | 10 | 1 Major 刻度 = ? Tick |

**FSchedulerRulerStyle：**

| 属性 | 类型 | 说明 |
|------|------|------|
| `TickColor` | FLinearColor | 刻度线颜色 |
| `TickThickness` | float | 刻度线粗细（像素） |
| `BackgroundColor` | FLinearColor | 背景色 |
| `TextColor` | FLinearColor | 大刻度值文字颜色 |
| `FontSize` | int32 | 刻度值字号 |

**绘制逻辑（OnPaint）：**

```
EffectiveTickPixel = MinorPixel / MinorTicks
可见起点 = ViewStartTick
可见刻度数 = LocalSize.X / EffectiveTickPixel
X_Offset = (Tick - ViewStartTick) * EffectiveTickPixel
```
- 使用 `TickLevel[ActiveLevelIndex]`，未配置时退至 Minor=1 / Major=10
- Tick 循环受 `MaxTickValue` 钳制上限

**坐标换算：**

```
Tick = ViewStartTick + Round(LocalX / EffectiveTickPixel)
```

**输入行为：**

| 手势 | Slate事件 | 效果 |
|------|------|------|
| 左键点击 | OnMouseButtonUp → OnClicked | SetCurrentTime(Tick) + Broadcast OnClicked |
| 左键拖拽 | OnMouseMove → OnClicked | 同上，Tick 变化时持续触发 |
| 右键拖拽 | OnMouseMove → OnDragged | ViewStartTick -= DeltaTick → RefreshUI.Broadcast |
| 滚轮 | OnMouseWheel → ZoomTickLevel | ActiveLevelIndex -= ScrollDelta |

**动作入口（UMG 蓝图事件）：**

| 委托 | 触发 | 参数 |
|------|------|------|
| `OnClicked` | 左键点击/拖拽 | `int64 Tick` |
| `OnDragged` | 右键拖拽 | `int64 DeltaTick` |
| `OnScrolled` | 滚轮 | `int32 ScrollDelta` |

**内部函数：**

| 函数 | 功能 |
|------|------|
| `HandleSetCurrentTime(Tick)` | Broadcast OnClicked + `CachedSubsystem->SetCurrentTime(Tick)` |
| `ScrollRuler()` | 绑定至 RefreshUI，Invalidate 刻度尺 |
| `ZoomTickLevel(Delta)` | `ActiveLevelIndex -= Delta` → Invalidate |

---

### TrackPanel —— Owner/Task 轨道面板

**定位：** BodyLeft + BodyRight 区的行列表，每行含 Title（名称+箭头+删除）和 Body（Keyframe 渲染面片）。由 Subsystem 直接管理增删，Widget 仅负责初始化注入容器。

> 详细控件层级、数据结构(FTrack/FTaskTrackEntry)、增量流、折叠逻辑见 **[TRACKPANEL_DSL.md](TRACKPANEL_DSL.md)**。

**蓝图属性（Track，均位于 USchedulerWidget）：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|------|------|
| `TrackHeight` | float | 24 | 轨道行高 |
| `OwnerTrackColor` | FLinearColor | (0.08,0.08,0.08) | OwnerTrack 背景色 |
| `TaskTrackColor` | FLinearColor | (0.12,0.12,0.12) | TaskTrack 背景色 |
| `TrackTextColor` | FLinearColor | (0.9,0.9,0.9) | 标题文字颜色 |
| `TrackBorderColor` | FLinearColor | (0.2,0.2,0.2) | 行描边颜色 |
| `TrackTitleMargin` | FMargin | (1,1,1,1) | Title 边框四边 |
| `TrackBodyMargin` | FMargin | (1,1,0,1) | Body 边框（右=0） |
| `TrackFontSize` | int32 | 10 | 标题字号 |

**垂直滚动同步：** TitleScrollBox ↔ BodyScrollBox 双向 SetScrollOffset，`bIsScrollSyncing` 门禁防乒乓。

---

### Playhead —— 游标控件

**定位：** SOverlay 覆盖整个 USchedulerWidget 的独立 Slate 控件，OnPaint 直接绘制竖线 + 时间标签。HitTestInvisible，不拦截鼠标事件。

**文件：** `Public/UI/Playhead/SSchedulerPlayhead.h`

| 属性 | 类型 | 默认值 | 说明 |
|------|------|------|------|
| `PlayheadWidth` | float | 2 | 竖线宽度 |
| `PlayheadColor` | FLinearColor | Red | 竖线颜色 |
| `PlayheadFontSize` | int32 | 8 | 时间标签字号 |
| `PlayheadFontColor` | FLinearColor | Red | 时间标签颜色 |

**更新链路：** `OnTimeChanged.AddDynamic → USchedulerWidget::UpdatePlayhead`，X 坐标 = `LeftSidebarWidth + (CurrentTime - ViewStartTick) * EffectiveTickPixel`，超出可见范围时 `SetVisibility(Collapsed)`。

---

## UI 交互链路

```
SSchedulerRuler 输入
    │
    ├─ 左键点击/拖拽 → OnClicked(Tick)
    │   ├─ USchedulerRuler::HandleSetCurrentTime → CachedSubsystem->SetCurrentTime(Tick)
    │   └─ USchedulerWidget::HandleRulerClicked → Subsystem->SetCurrentTime(Tick)
    │
    ├─ 右键拖拽 → OnDragged(DeltaTick)
    │   ├─ ViewStartTick -= DeltaTick + Invalidate（滚动）
    │   └─ USchedulerWidget::HandleRulerDragged → RefreshUI.Broadcast()
    │       └─ USchedulerRuler::ScrollRuler() → Invalidate
    │
    └─ 滚轮 → ZoomTickLevel(Delta)
        ├─ ActiveLevelIndex -= Delta + Invalidate（缩放）
        └─ OnScrolled(Delta) → USchedulerWidget::HandleRulerScrolled
            ├─ ZoomTickLevel(Delta)
            └─ RefreshUI.Broadcast()
```

---

## 名词定义

### Tick —— Scheduler 插件最小时间单位

| 属性 | 说明 |
|------|------|
| 本质 | 纯计量刻度单位，不赋予任何现实时间属性（秒/毫秒/帧） |
| 运算 | 整数，CurrentTime ± 1Tick = 时间戳 ± 1 |
| 类型 | 涉及 Tick 计数的变量统一使用 `int64` |
| 示例 | `SetCurrentTime(100)` 表示时刻 = 100 Tick，不代表 100 秒或 100 帧 |

---

## 接口清单

| 接口 | 函数 | 说明 |
|------|------|------|
| `ITaskInterface` | `ExecuteTask(Alpha, bIsForward, ClipIndex)` | Task → Owner 时刻变更通知，Alpha 为区间归一化进度 0.0~1.0，BlueprintNativeEvent |
| `ITaskInterface` | `DestroyTask()` | Task → Owner 销毁通知 |
| `ITaskInterface` | `RemoveKeyframe(KeyframeIndex)` | Task → Owner 关键帧移除通知，参数为被移除的索引 |

> 所有接口调用均经 `IsValid(TaskOwner)` 防护——TaskOwner（如 Component）可能随其 Outer（Actor）提前销毁，裸指针判空不足以检测 UE 对象生命周期。

---

## 运行约束

| 约束 | 说明 |
|------|------|
| CurrentTime >= 0 | `SetCurrentTime` 负值被拦截，`CurrentTimeMinusMinus` 触底钳位为0 |
| 相同值不广播 | `SetCurrentTime` 检测 NewCurrentTime == CurrentTime 时 early return |
| 触底静默 | `CurrentTimeMinusMinus` 已在0时静默 return，不递减不广播 |
| 初始化广播 | `initializationSubsystem` 始终广播 (0, true) |
| 线程 | 所有操作在 GameThread，委托为 DynamicMulticast 不跨线程 |
| UI 无 Tick | SchedulerWidget 仅十字分割布局，不参与渲染刷新循环 |
| 子项自治 | SchedulerWidget 四个 Slot 中控件独立运作 |
| TaskOwner 必填 | `CreateTask` 蓝图节点必须连线有效 TaskOwner，否则编译不过 |
| Task 唯一性 | 按 OwnerName 管理，同一 OwnerName 下可有多个 Task |
| IsValid 防护 | 所有 ITaskInterface 调用均经 `IsValid(TaskOwner)` 防护——Component 可能随其 Outer Actor 先销毁 |
| 越界回调去重 | OnTimeChange 超出 Keyframes 边界时钳位广播——连续超出同侧仅触发一次，交替穿越可连续触发 |
| 归零正向 | SetCurrentTime(0) 和 CurrentTimeMinusMinus 归零时 bIsForward 强制为 true |
| DestroyOwner 不循环 DestroyTask | DestroyTask 在末 Task 移除时删 TaskMap 键，循环调会导致后续 Task 静默跳过 |

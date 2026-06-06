# SchedulerPluginDev · 架构DSL

> 更新：2026-06-06
> 用途：项目长久记忆，记录功能所有者、广播、接口与委托链
> 维护规则：每次增删委托/接口/函数签名/UI控件后同步更新本文

---

## 模块树

```
Scheduler (插件模块)
├── USchedulerSubsystem : UWorldSubsystem   [内核——时刻调度中心 + Task工厂/池]
├── USchedulerTask : UObject                [任务数据单元——关键帧 + 时刻回调]
│   └── FClipIndex : USTRUCT               [关键帧区间——LastIndex/NextIndex]
├── ITaskInterface / UTaskInterface         [接口层——Task通知Owner的唯一回调]
├── ASchedulerTaskPool : AActor             [任务池——占位，已废弃，功能收敛至Subsystem]
├── USchedulerWidget : UWidget              [UI层——总容器，蓝图唯一暴露口]
│   └── SSchedulerWidget : SCompoundWidget  [Slate——十字分割布局]
└── USchedulerRuler : UWidget               [UI层——水平刻度尺，支持滚动/缩放/点击]
    └── SSchedulerRuler : SCompoundWidget   [Slate——刻度绘制+输入处理]
```

---

## 接口层

### ITaskInterface —— Task → Owner 通知接口

**定位：** 替代原有的 `FExecuteTask` 多播委托。Task 对 Owner 是唯一的，接口回调语义更精确。蓝图可直接实现。

**文件：** `Public/TaskInterface.h`

```
UINTERFACE(Blueprintable)
class UTaskInterface : public UInterface

class ITaskInterface
{
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void ExecuteTask(int64 NewCurrentTime, bool bIsForward, FClipIndex ClipIndex);
};
```

**调用方式（C++）：**

```
ITaskInterface::Execute_ExecuteTask(TaskOwner, NewCurrentTime, bIsForward, ClipIndex);
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
| 文件 | `Public/SchedulerTask.h` |

**成员变量：**

| 变量 | 类型 | 说明 |
|------|------|------|
| `Keyframes` | `TArray<int64>` | 关键帧时间点数组，有序 |
| `TaskName` | `FString` | 任务名称 |
| `TaskOwnerName` | `FString` | 拥有者名称（TaskMap 的分组键） |
| `TaskOwner` | `UObject*` | 拥有者指针（即 Outer，实现 ITaskInterface） |

**函数：**

| 函数 | 访问 | 类别 | 功能 |
|------|------|------|------|
| `OnTaskInitialized()` | public | — | 初始化钩子，预留 TaskTrack::Create 调用 |
| `OnDestory()` | public | — | 销毁钩子，预留 UI 销毁逻辑 |
| `OnTimeChange(int64, bool)` | public | — | 时刻变更回调：二分查找 → 构造 ClipIndex → ITaskInterface::Execute_ExecuteTask |
| `AddKeyframe(int64, const TArray<int64>&, int32&, bool&)` | public, BlueprintCallable | Scheduler\|Task | 拷贝 InKeyframes 到成员 Keyframes，二分查找插入/去重，TODO UI 更新 |
| `DeleteKeyframe(int64, int32&)` | public | — | 按值查找并移除关键帧，返回原始索引，不对蓝图公开 |

**OnTimeChange 流程：**

```
空数组检查 → 越界检查（[0] / Last()）
  → 二分查找：Left = 首个 >= InCurrentTime 的索引
  → 构造 ClipIndex（精确命中按方向，区间内统一 [Left-1, Left]）
  → ITaskInterface::Execute_ExecuteTask(TaskOwner, ...)
```

---

## 内核层

### USchedulerSubsystem —— 时刻调度中心 + Task 工厂/池

**定位：** 内核变量 `CurrentTime` 完全私有；Task 的创建、管理池、OnTimeChanged 绑定统一收敛于此。

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
    └→ OnTimeChanged.Broadcast(CurrentTime, bIsForward)

CurrentTimePlusPlus()
    └→ OnTimeChanged.Broadcast(CurrentTime, true)

CurrentTimeMinusMinus()
    ├─ 触底：CurrentTime < 0 → CurrentTime=0, return
    └→ OnTimeChanged.Broadcast(CurrentTime, false)
```

**函数清单：**

| 函数 | 访问 | 类别 | 功能 | 副作用 |
|------|------|------|------|------|
| `initializationSubsystem()` | public, BlueprintCallable | Scheduler | 重置 CurrentTime=0，广播 | 触发 OnTimeChanged |
| `GetCurrentTime() const` | public, BlueprintPure | Scheduler\|TimeChange | 返回当前时刻 | 无 |
| `SetCurrentTime(int64)` | public, BlueprintCallable | Scheduler\|TimeChange | 跳转到指定时刻 | 触发 OnTimeChanged（有门禁） |
| `CurrentTimePlusPlus()` | public, BlueprintCallable | Scheduler\|TimeChange | 前进1时刻 | 触发 OnTimeChanged |
| `CurrentTimeMinusMinus()` | public, BlueprintCallable | Scheduler\|TimeChange | 后退1时刻，触底钳位0 | 条件触发 OnTimeChanged |
| `CreateTask(FString, FString, UObject*)` | public, BlueprintCallable | Scheduler\|Task | 工厂创建 Task 并入池 | 绑定 OnTimeChanged |
| `DeleteTask(USchedulerTask*)` | public, BlueprintCallable | Scheduler\|Task | 从 TaskMap 移除 Task，清理空分组 | 返回 bool，仅移除引用 |

**CreateTask 流程：**

```
NewObject<USchedulerTask>(TaskOwner)
  → 赋值 TaskName, TaskOwnerName, TaskOwner
  → OnTaskInitialized()
  → TaskMap.FindOrAdd(TaskOwnerName).Add(NewTask)
  → OnTimeChanged.AddDynamic(NewTask, &USchedulerTask::OnTimeChange)
  → return NewTask
```

**DeleteTask 流程：**

```
IsValid(Task) 门禁
  → TaskMap.Find(TaskOwnerName) 查找分组
  → OwnerTasks->Remove(Task) 移除
  → 分组为空则 TaskMap.Remove(TaskOwnerName)
  → return true（GC 自动回收 Task，委托自动解绑）
```

**TaskMap：**

| 属性 | 类型 | 说明 |
|------|------|------|
| `TaskMap` | `TMap<FString, TArray<USchedulerTask*>>` | 按 OwnerName 分组管理所有 Task |

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
    ITaskInterface::Execute_ExecuteTask(TaskOwner, Time, Dir, ClipIndex)
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
| UMG 包装（蓝图可见） | `USchedulerWidget` | `Public/USchedulerWidget.h` |
| Slate 实现 | `SSchedulerWidget` | `Public/SSchedulerWidget.h` |

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
| `HeadRight` | 右上 | `.HeadRight(SNew(SSchedulerRuler))` |
| `BodyLeft` | 左下 | `.BodyLeft(SNew(...))`（预留） |
| `BodyRight` | 右下 | `.BodyRight(SNew(...))`（预留） |

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
| UMG 包装 | `USchedulerRuler` | `Public/USchedulerRuler.h` |
| Slate 实现 | `SSchedulerRuler` | `Public/SSchedulerRuler.h` |
| 类型定义 | `FTickLevel` / `FSchedulerRulerStyle` | `Public/SchedulerRulerTypes.h` |

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
| `ITaskInterface` | `ExecuteTask(NewCurrentTime, bIsForward, ClipIndex)` | Task → Owner 时刻变更通知，BlueprintNativeEvent |

---

## 运行约束

| 约束 | 说明 |
|------|------|
| CurrentTime >= 0 | `SetCurrentTime` 负值被拦截，`CurrentTimeMinusMinus` 触底钳位为0 |
| 相同值不广播 | `SetCurrentTime` 检测 NewCurrentTime == CurrentTime 时 early return |
| 触底静默 | `CurrentTimeMinusMinus` 钳位0时不触发广播 |
| 初始化广播 | `initializationSubsystem` 始终广播 (0, true) |
| 线程 | 所有操作在 GameThread，委托为 DynamicMulticast 不跨线程 |
| UI 无 Tick | SchedulerWidget 仅十字分割布局，不参与渲染刷新循环 |
| 子项自治 | SchedulerWidget 四个 Slot 中控件独立运作 |
| TaskOwner 必填 | `CreateTask` 蓝图节点必须连线有效 TaskOwner，否则编译不过 |
| Task 唯一性 | 按 OwnerName 管理，同一 OwnerName 下可有多个 Task |

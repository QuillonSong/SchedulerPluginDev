# SchedulerPluginDev · 架构DSL

> 更新：2026-06-04
> 用途：项目长久记忆，记录功能所有者、广播、接口与委托链
> 维护规则：每次增删委托/接口/函数签名/UI控件后同步更新本文

---

## 模块树

```
Scheduler (插件模块)
├── USchedulerSubsystem : UWorldSubsystem   [内核——时刻调度中心]
├── USchedulerTask : UObject                [任务数据单元——占位]
├── ASchedulerTaskPool : AActor             [任务池——占位]
├── USchedulerWidget : UWidget              [UI层——总容器，蓝图唯一暴露口]
│   └── SSchedulerWidget : SCompoundWidget  [Slate——十字分割布局]
└── USchedulerRuler : UWidget               [UI层——水平刻度尺，支持滚动/缩放/点击]
    └── SSchedulerRuler : SCompoundWidget   [Slate——刻度绘制+输入处理]
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

**蓝图属性（Ruler）——直接暴露在本类，选 SchedulerWidget 即改：**

| 属性 | 类型 | 默认值 | 说明 |
|------|------|------|------|
| `RulerStyle` | `FSchedulerRulerStyle` | — | 刻度线颜色、粗细、背景色 |
| `RulerMajorLength` | float | 15 | 大刻度线像素高度 |
| `RulerMinorLength` | float | 5 | 小刻度线像素高度 |
| `RulerMinorPixel` | float | 10 | 每 Minor 格子固定像素宽度——缩放不改此值 |
| `RulerTickOffset` | float | 0 | 刻度起始 Y 偏移，上方留白 |
| `RulerMaxTickValue` | int64 | 0 | 刻度轴最大 Tick 值，0=不限制 |
| `RulerTickLevel` | `TArray<FTickLevel>` | [] | 缩放层级数组 |

**事件（UMG）：**

| 委托 | 说明 |
|------|------|
| `RefreshUI` | UI 同步刷新信号——右键拖拽、滚轮时广播，子控件绑定此委托刷新显示 |

**四个 Slot：**

| Slot 名 | 位置 | 填充方式（C++） |
|------|------|------|
| `HeadLeft` | 左上 | `.HeadLeft(SNew(...))` |
| `HeadRight` | 右上 | `.HeadRight(SNew(...))` | 构造阶段自动挂载内部刻度尺 |
| `BodyLeft` | 左下 | `.BodyLeft(SNew(...))` |
| `BodyRight` | 右下 | `.BodyRight(SNew(...))` |

**生命周期约束：**

| 约束 | 说明 |
|------|------|
| 无 Tick | `SSchedulerWidget` 不覆写 `Tick`，无主动渲染刷新 |
| 按需重绘 | 仅 `bIsDrawCross` 变更时 `Invalidate(EInvalidateWidgetReason::Paint)` |
| 按需重排 | `HeadHeight`/`LeftSidebarWidth` 变更时 `InvalidateLayout` |
| 子项自治 | Slot 中填充的 Widget 自行管理交互/渲染/生命周期 |
| Slate 释放 | `USchedulerWidget::ReleaseSlateResources` 已覆写，Reset MySlateWidget |

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
| `MajorLength` | float | 15 | 大刻度线像素高度，从基线向下延伸 |
| `MinorLength` | float | 5 | 小刻度线像素高度，从基线向下延伸 |
| `MinorPixel` | float | 10 | 每 Minor 格子固定像素宽度——缩放不改此值 |
| `TickOffset` | float | 0 | 刻度起始 Y 偏移，上方留白 |
| `MaxTickValue` | int64 | 0 | 刻度轴最大 Tick 值，0=不限制 |
| `ActiveLevelIndex` | int32 | 0 | 当前 TickLevel 索引，由滚轮/ZoomTickLevel 控制 |
| `ViewStartTick` | int64 | 0 | 可见区域起始 Tick，右键拖拽滚动修改 |
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
- 大刻度下方绘制刻度值标签，水平居中

**坐标换算（所有输入）：**

```
Tick = ViewStartTick + Round(LocalX / EffectiveTickPixel)
```

**输入行为：**

| 手势 | Slate事件 | 效果 |
|------|------|------|
| 左键点击 | OnMouseButtonUp → OnClicked | SetCurrentTime(Tick) + Broadcast OnClicked |
| 左键拖拽 | OnMouseMove → OnClicked | 同上，Tick 变化时持续触发 |
| 右键拖拽 | OnMouseMove → OnDragged | ViewStartTick -= DeltaTick → 刻度轴滚动 → RefreshUI.Broadcast |
| 滚轮 | OnMouseWheel → ZoomTickLevel | ActiveLevelIndex -= ScrollDelta → 切缩放层级 |

**动作入口（UMG 蓝图事件）：**

| 委托 | 触发 | 参数 |
|------|------|------|
| `OnClicked` | 左键点击/拖拽（每次 Tick 变化） | `int64 Tick` |
| `OnDragged` | 右键拖拽 | `int64 DeltaTick` — 增量（正=右拖） |
| `OnScrolled` | 滚轮 | `int32 ScrollDelta` — 滚动量（正=上滚） |

**内部函数：**

| 函数 | 功能 |
|------|------|
| `HandleSetCurrentTime(Tick)` | Broadcast OnClicked + `CachedSubsystem->SetCurrentTime(Tick)` |
| `ScrollRuler()` | 绑定至 RefreshUI，Invalidate 刻度尺 |
| `ZoomTickLevel(Delta)` | `ActiveLevelIndex -= Delta` → Invalidate |

**系统集成：**

- `CachedSubsystem` 在 `RebuildWidget` 时缓存 `World->GetSubsystem<USchedulerSubsystem>()`，避免每次查找
- UMG 面板隐藏（无 `GetPaletteCategory`），参数收敛至 `USchedulerWidget`

**生命周期约束：**

| 约束 | 说明 |
|------|------|
| 无 Tick | 仅属性/输入变更时 Invalidate Paint |
| MinorPixel 下限 | 最小 1px |
| Tick 坐标 | 所有 X→Tick 换算统一加 ViewStartTick 偏移 |
| 缩放安全 | ActiveLevelIndex 始终钳位到 [0, TickLevel.Num()-1] |

---

## 任务层

### USchedulerTask —— 任务数据单元

**定位：** 单个调度任务的 UObject 封装。当前为占位类，功能待实现。

| 属性 | 值 |
|------|-----|
| 父类 | UObject |
| 文件 | `Public/SchedulerTask.h` |
| 状态 | 占位——无成员、无函数 |

### ASchedulerTaskPool —— 任务池

**定位：** 管理 USchedulerTask 集合的 Actor 容器。当前为占位类，功能待实现。

| 属性 | 值 |
|------|-----|
| 父类 | AActor |
| 文件 | `Public/SchedulerTaskPool.h` |
| 状态 | 占位——仅含构造函数、BeginPlay、Tick 骨架 |
| 注意 | Tick 当前为空，功能落地后应评估是否需要禁用以避免无意义帧开销 |

---

## 内核层

### USchedulerSubsystem —— 时刻调度中心

**定位：** 内核变量 `CurrentTime` 完全私有，所有读写经函数门禁。

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
    └→ OnTimeChanged.Broadcast(0, true)                    [初始化归零，方向约定为true]

SetCurrentTime(NewCurrentTime)
    ├─ 门禁：NewCurrentTime == CurrentTime → return         [相同值不广播]
    ├─ 门禁：NewCurrentTime < 0 → return                    [负数拦截]
    └→ OnTimeChanged.Broadcast(CurrentTime, bIsForward)     [方向由新旧值比较判定]

CurrentTimePlusPlus()
    └→ OnTimeChanged.Broadcast(CurrentTime, true)           [固定前进]

CurrentTimeMinusMinus()
    ├─ 触底：CurrentTime < 0 → CurrentTime=0, return        [不广播，静默钳位]
    └→ OnTimeChanged.Broadcast(CurrentTime, false)          [固定后退]
```

**订阅方（任意类型均可绑定）：**

| 绑定方式 | 代码 |
|------|------|
| C++ | `Subsystem->OnTimeChanged.AddDynamic(this, &UMyClass::Handler);` |
| Blueprint | 获取Subsystem → Bind Event to OnTimeChanged |

**函数清单：**

| 函数 | 访问 | 类别 | 功能 | 副作用 |
|------|------|------|------|------|
| `initializationSubsystem()` | public, BlueprintCallable | Scheduler | 重置 CurrentTime=0，广播通知 | 触发 OnTimeChanged |
| `GetCurrentTime() const` | public, BlueprintPure | Scheduler\|TimeChange | 返回当前时刻 | 无 |
| `SetCurrentTime(int64)` | public, BlueprintCallable | Scheduler\|TimeChange | 跳转到指定时刻 | 触发 OnTimeChanged（有门禁） |
| `CurrentTimePlusPlus()` | public, BlueprintCallable | Scheduler\|TimeChange | 前进1时刻 | 触发 OnTimeChanged |
| `CurrentTimeMinusMinus()` | public, BlueprintCallable | Scheduler\|TimeChange | 后退1时刻，触底钳位0 | 条件触发 OnTimeChanged |

**核心变量：**

| 变量 | 位置 | 可见性 | 说明 |
|------|------|------|------|
| `CurrentTime` | `USchedulerSubsystem` private | 仅C++ Subsystem内部 | 内核变量，禁止外部直读写；所有读写通过 Get/Set 函数门禁 |

---

## 接口清单

> 当前无接口。时间通知已改用委托模式，不再使用 ITimeChange。

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
    ┌─────────────────────────────────────┐
    │  任意类型订阅者                       │
    │  AActor / UWidget / UUserWidget /    │
    │  UObject / Blueprint                 │
    │  （含 SchedulerWidget 子项中的控件）   │
    └─────────────────────────────────────┘
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

## 运行约束

| 约束 | 说明 |
|------|------|
| CurrentTime >= 0 | `SetCurrentTime` 负值被拦截，`CurrentTimeMinusMinus` 触底钳位为0 |
| 相同值不广播 | `SetCurrentTime` 检测 NewCurrentTime == CurrentTime 时 early return |
| 触底静默 | `CurrentTimeMinusMinus` 钳位0时不触发广播，视为无效操作 |
| 初始化广播 | `initializationSubsystem` 始终广播 (0, true)，即使 CurrentTime 已是0 |
| 线程 | 所有操作在 GameThread，委托为 DynamicMulticast 不跨线程 |
| UI 无 Tick | SchedulerWidget 仅十字分割布局+静态分割线，不参与任何渲染刷新循环 |
| 子项自治 | SchedulerWidget 的四个 Slot 中控件独立运作，SchedulerWidget 仅提供几何约束 |

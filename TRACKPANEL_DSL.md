# TrackPanel · 架构DSL

> 更新：2026-06-09
> 用途：Scheduler 插件 Track/Keyframe/Playhead 架构设计与实现文档
> 版本：v1.1（DestroyOwner + SyncKeyframes + 生命周期防护）

---

## 前置规则：蓝图暴露约束

| 规则 | 说明 |
|------|------|
| **Subsystem 函数** | 唯一对外 C++/蓝图调用入口，所有公开函数均 BlueprintCallable |
| **USchedulerWidget 变量** | 唯一对外蓝图配置入口，Track/Ruler 等视觉属性均在此 UPROPERTY |
| **Slate 层（SSchedulerTrack*）** | **禁止暴露给蓝图**，全部为 C++ internal |
| **委托** | `OnTimeChanged`、`RefreshUI` 可蓝图绑定；数据变更由 Subsystem 直接处理，无额外委托 |
| **DSL 维护** | 每次新增/修改接口、委托、函数签名后同步更新本文 |

---

## 模块树

```
Scheduler (插件模块)
├── USchedulerSubsystem : UWorldSubsystem   [内核——时刻调度 + Task池 + Track池]
├── USchedulerTask : UObject                [任务数据——关键帧 + 时刻回调]
├── USchedulerWidget : UWidget              [UI层——总容器，蓝图唯一暴露口]
│   └── SSchedulerWidget : SCompoundWidget  [Slate——十字分割布局]
├── USchedulerRuler : UWidget               [UI层——水平刻度尺UMG包装]
├── SSchedulerRuler : SCompoundWidget       [UI层——水平刻度尺Slate实现]
├── SSchedulerPlayhead : SCompoundWidget    [UI层——游标竖线 + 时间标签]
├── SSchedulerTrackTitle : SCompoundWidget  [Track行——BodyLeft单行标题(箭头+文字+删除)]
├── SSchedulerTrackBody : SCompoundWidget   [Track行——BodyRight单行体(Keyframe渲染)]
├── FTrack (struct)                         [OwnerTrack——Title+Body+子TaskTrack列表]
└── FTaskTrackEntry (struct)                [TaskTrack行——Title+Body+Task指针]
```

---

## 文件清单

| 文件 | 说明 |
|------|------|
| `Public/Core/SchedulerSubsystem.h` + `.cpp` | FTrack / FTaskTrackEntry + TrackMap + 增量管理 |
| `Public/UI/USchedulerWidget.h` + `.cpp` | 蓝图属性 + ScrollBox 容器 + 滚动同步 |
| `Public/UI/Track/SSchedulerTrackTitle.h` + `.cpp` | BodyLeft 区单行标题 Widget（箭头+文字+删除） |
| `Public/UI/Track/SSchedulerTrackBody.h` + `.cpp` | BodyRight 区单行体 Widget（Keyframe 渲染） |
| `Public/UI/Playhead/SSchedulerPlayhead.h` + `.cpp` | 游标竖线 + 时间标签 |
| `Public/UI/Ruler/SSchedulerRuler.h` + `.cpp` | 水平刻度尺 Slate 实现 |
| `Public/UI/Ruler/USchedulerRuler.h` + `.cpp` | 刻度尺 UMG 包装 |
| `Public/UI/Ruler/SchedulerRulerTypes.h` | FTickLevel + FSchedulerRulerStyle 类型 |

### 依赖资产

| 资产 | 用途 |
|------|------|
| `Content/T_UI_Arrow.uasset` | 折叠箭头图标（►，展开时 SetRenderTransform 旋转 90°→▼） |
| `Content/T_UI_Close.uasset` | 删除按钮图标（✖） |

---

## 控件层级

### SSchedulerTrackTitle

```
SButton("NoBorder", ContentPadding=0, OnClicked=折叠展开)
  └─ SBox(HeightOverride=RowHeight)
       └─ SOverlay
            ├─ SImage(描边, DrawAs=Border, Margin=TitleMargin)
            ├─ SImage(底色, FSlateColorBrush(White), Padding=TitleMargin)
            ├─ SImage(箭头►, T_UI_Arrow, 仅OwnerTrack, 展开时旋转90°)
            ├─ STextBlock(OwnerName/TaskName, ColorAndOpacity=TextColor)
            └─ SButton("NoBorder", OnClicked=删除)
                 └─ SImage(✖, T_UI_Close)
```

### SSchedulerTrackBody

```
SBox(HeightOverride=RowHeight)
  └─ SOverlay
       ├─ SImage(描边色, DrawAs=Border, Margin=BodyMargin)
       ├─ SImage(底色, FSlateColorBrush(White), Padding=BodyMargin)
       └─ SOverlay(KeyframeContainer, Phase 3)
```

---

## 数据结构

### FTaskTrackEntry —— TaskTrack 行

```cpp
struct FTaskTrackEntry
{
    TSharedPtr<SSchedulerTrackTitle> Title;
    TSharedPtr<SSchedulerTrackBody>  Body;
    USchedulerTask* Task = nullptr;   // DeleteTask 时精确匹配
};
```

### FTrack —— OwnerTrack 行 + 池

```cpp
struct FTrack
{
    TSharedPtr<SSchedulerTrackTitle> Title;
    TSharedPtr<SSchedulerTrackBody>  Body;
    TArray<FTaskTrackEntry> TaskTracks;
    int32 OwnerRowIndex = INDEX_NONE;   // 插入位置索引
    bool  bIsCollapsed = false;         // 折叠态
};
```

### TrackMap（Subsystem 管理）

```cpp
TMap<FString, TArray<USchedulerTask*>> TaskMap;  // 数据池
TMap<FString, FTrack> TrackMap;                  // UI池，镜像键
```

---

## 增量数据流

### CreateTask

```
CreateTask(TaskName, OwnerName, Owner)
  ├─ NewObject<USchedulerTask> → 入 TaskMap[OwnerName]
  ├─ TrackMap 无 OwnerName?
  │    └─ CreateOwnerTrackWidgets → AddSlot Title/Body
  │         .DisplayName("▼ OwnerName")        ← 初始展开
  │         .OnTrackClicked → 切换折叠 + 箭头
  │         TrackMap.Add(OwnerName, FTrack)
  └─ CreateTaskTrackWidgets → InsertSlot(Title+Body, 缩进, TaskColor)
       .bIsChild=true, .IndentWidth=20
       .KeyframesPtr=&Task->Keyframes
       若 Owner 已折叠 → SetVisibility(Collapsed)
```

### DestroyTask

```
DestroyTask(Task)
  ├─ TaskMap[OwnerName].Remove(Task)
  ├─ 匹配 FTaskTrackEntry.Task → DestroyTaskTrackWidgets → RemoveSlot
  ├─ OwnerTasks 为空?
  │    └─ DestroyOwnerTrackWidgets → RemoveSlot → TrackMap.Remove
  └─ Task->OnDestroy()
       ├─ OnTimeChanged.RemoveDynamic
       ├─ ITaskInterface::Execute_DestroyTask(TaskOwner)
       └─ MarkAsGarbage()
```

### DestroyOwner（v1.1.0 新增）

```
DestroyOwner(OwnerName)
  ├─ TaskMap.Find(OwnerName)——无则 return false
  ├─ TrackMap.Find(OwnerName)——可能为 nullptr（UI 未初始化）
  ├─ for Task in *OwnerTasks:
  │    └─ IsValid(Task) → Task->OnDestroy()
  │         ├─ bIsOnDestroy 防重入门禁
  │         ├─ RemoveDynamic + ITaskInterface 通知 + MarkAsGarbage
  ├─ if OwnerTrack:
  │    └─ DestroyOwnerTrackWidgets → 遍历子 TaskTrack + RemoveSlot + 索引调整
  │    └─ TrackMap.Remove(OwnerName)
  └─ TaskMap.Remove(OwnerName)——延后至 OnDestroy 完成后
  return true
```

> 注意：DestroyOwner **不循环调用 DestroyTask**——DestroyTask 在最后一个 Task 移除时会从 TaskMap 删键，循环中后续 Task 的 `TaskMap.Find` 返回空导致静默跳过。DestroyOwner 直接管理数据销毁与 Map 清理，保证批量完整性。

### 折叠/展开

```
点击 OwnerTrack Title（SButton::OnClicked）
  ├─ bIsCollapsed = !bIsCollapsed
  ├─ 遍历 TaskTracks → SetVisibility(Collapsed / Visible)
  └─ Title->SetExpanded(!bIsCollapsed)
       └─ 折叠: FSlateRenderTransform()       → ► 0°
          展开: FSlateRenderTransform(Quat2D(90°)) → ▼

点击 ✖ 按钮（OnDeleteClicked）
  ├─ OwnerTrack ✖ → 删除该 Owner 下所有 Task → OwnerTrack 移除
  └─ TaskTrack  ✖ → DeleteTask(Task) → 分组清空则 OwnerTrack 移除
```

---

## 蓝图暴露属性（USchedulerWidget）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|------|------|
| `TrackHeight` | float | 24 | 轨道行高 |
| `OwnerTrackColor` | FLinearColor | (0.08,0.08,0.08) | OwnerTrack 背景色 |
| `TaskTrackColor` | FLinearColor | (0.12,0.12,0.12) | TaskTrack 背景色 |
| `TrackTextColor` | FLinearColor | (0.9,0.9,0.9) | 标题文字颜色 |
| `TrackBorderColor` | FLinearColor | (0.2,0.2,0.2) | 行描边颜色 |
| `TrackTitleMargin` | FMargin | (1,1,1,1) | Title 行边框四边 |
| `TrackBodyMargin` | FMargin | (1,1,0,1) | Body 行边框（右=0） |
| `TrackFontSize` | int32 | 10 | 标题字号 |

### Phase 3 预留

| 属性 | 类型 | 默认值 |
|------|------|------|
| `KeyframeSize` | float | 10 |
| `UnCheckedColor` | FLinearColor | 白色 |
| `CheckedColor` | FLinearColor | 蓝色 |

---

## 垂直滚动同步

```
TitleScrollBox                    BodyScrollBox
  OnUserScrolled(Offset)            OnUserScrolled(Offset)
      │                                  │
      └── SetOffset(Body) ←─┐  ┌── SetOffset(Title) ──┘
                              │  │
                         bIsScrollSyncing 门禁防乒乓
```

---

## 运行约束

| 约束 | 说明 |
|------|------|
| 增量 | 所有 Track 增删均为增量操作，无全量刷新 |
| 同步 | Subsystem 直接管理 Track 控件，Widget 只负责初始化注入 |
| 折叠 | Visibility = Collapsed，整行（Title + Body）同步 |
| 索引 | OwnerRowIndex 自动维护插入/删除后的偏移 |
| 线程 | 所有操作在 GameThread |
| 无 Tick | 所有 Track 相关 Widget 不覆写 Tick |

---

## Phase 3 —— Keyframe 渲染与交互（已完成）

### 渲染
- SCanvas + SImage + FSlateBrush(KeyframeTexture) 定位渲染
- 位置 = `(Tick - ViewStartTick) * EffectiveTickPixel`，与 Ruler 水平对齐
- Y = `(RowHeight - KeyframeSize) * 0.5` 垂直居中

### 交互
- 左键点击 → 选中（单选，CheckedColor 高亮）
- 右键点击选中 Keyframe → 删除 → `OnKeyframeDelete(Tick)` → `Task::RemoveKeyframe(Tick)` → `ITaskInterface::RemoveKeyframe(Index)` → `RefreshAllKeyframes()`

### 蓝图属性（USchedulerWidget）
| 属性 | 类型 | 默认值 |
|------|------|--------|
| `KeyframeSize` | float | 10 |
| `UnCheckedColor` | FLinearColor | White |
| `CheckedColor` | FLinearColor | (0.2, 0.5, 1.0) |
| `KeyframeTexture` | UTexture2D* | nullptr |

### 已知问题
- `[FIXME]`: Keyframe 属性(Size/Color/Texture)修改后渲染不更新——SyncKeyframeState→SetKeyframeParams→RefreshKeyframes 链路待排查
- 聚合功能（缩放时重叠 >50% 合并为菱形）未实现
- Circle/Triangle 原生画刷形未实现（可 KeyframeTexture 自定义贴图替代）

---

## Task 生命周期

```
CreateTask → NewObject → set Subsystem → OnTaskInitialized()
  ├─ TaskMap.FindOrAdd(OwnerName).Add(this)
  ├─ OnTimeChanged.AddDynamic
  ├─ 新 Owner → CreateOwnerTrackInternal → CreateOwnerTrackWidgets
  └─ CreateTaskTrackInternal → CreateTaskTrackWidgets

DestroyTask → DestroyTaskTrackWidgets → OwnerTasks.Remove
  → Task->OnDestroy()
      ├─ bIsOnDestroy 门禁（防重入）
      ├─ OnTimeChanged.RemoveDynamic
      ├─ ITaskInterface::Execute_DestroyTask(TaskOwner)
      └─ MarkAsGarbage()
  → 分组清空 → DestroyOwnerTrackWidgets → TrackMap.Remove

DestroyOwner (批量) → for 所有 Task → OnDestroy()
  → DestroyOwnerTrackWidgets → TrackMap.Remove
  → TaskMap.Remove(OwnerName)

SyncKeyframes (数据恢复) → 排序去重 → RefreshAllKeyframes
```

---

## Playhead

### SSchedulerPlayhead

SOverlay 覆盖整个 USchedulerWidget 的独立控件。OnPaint 直接绘制竖线 + 时间标签。

| 属性 | 类型 | 默认值 | 说明 |
|------|------|------|------|
| `PlayheadWidth` | float | 2 | 竖线宽度 |
| `PlayheadColor` | FLinearColor | Red | 竖线颜色 |
| `PlayheadFontSize` | int32 | 8 | 时间标签字号 |
| `PlayheadFontColor` | FLinearColor | Red | 时间标签颜色 |

### 更新链路

```
OnTimeChanged.AddDynamic → UpdatePlayhead(Time, Dir)
Ruler 滚动/缩放 → UpdatePlayhead(Sub->GetCurrentTime(), true)

X = LeftSidebarWidth + (CurrentTime - ViewStartTick) * EffectiveTickPixel
超出可见范围 → Collapsed 隐藏
HitTestInvisible → 不拦截鼠标事件
```

---

## 打包

| 属性 | 值 |
|------|-----|
| 插件类型 | Runtime |
| Editor 目标 | ✅ UnrealEditor-Win64-Development |
| Game 目标 | ✅ UnrealGame-Win64-Development |
| 已知坑位 | `.generated.h` 必须不带路径前缀；Editor PCH 隐式补全的头文件在 Game 构建中需显式 include |

## Phase 4 待实现

- Keyframe 聚合算法
- Keyframe 属性热更新修复（`[FIXME]` 标记位置）
- 自定义画刷形（Circle/Triangle）

---

## v1.1.0 变更记录

| 变更 | 说明 |
|------|------|
| `DestroyOwner` | 按 OwnerName 批量销毁全部 Task + Track，不循环调 DestroyTask 以避免 TaskMap 键中途失效 |
| `SyncKeyframes` | Save/Load 后外部数据恢复入口，排序去重 + UI 重绘 |
| `bIsOnDestroy` 防重入 | OnDestroy 新增门禁，防批量操作重复通知 |
| `IsValid(TaskOwner)` 防护 | ExecuteTask/DestroyTask/RemoveKeyframe 三处裸指针判空改为 IsValid，防 Component 随 Actor 提前销毁后悬空 |
| 版本编码 | Version 10100 = MAJOR×10000 + MINOR×100 + PATCH → 1.1.0 |

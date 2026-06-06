# TrackPanel · 架构DSL

> 更新：2026-06-06
> 用途：Track 面板模块的架构设计与实现文档
> 当前阶段：Phase 1+2 已完成（OwnerTrack + TaskTrack + 折叠展开）

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
├── SSchedulerRuler : SCompoundWidget       [UI层——水平刻度尺]
├── SSchedulerTrackTitle : SCompoundWidget  [Track行——BodyLeft单行标题]
├── SSchedulerTrackBody : SCompoundWidget   [Track行——BodyRight单行体]
├── FTrack (struct)                         [OwnerTrack——Title+Body+子TaskTrack列表]
└── FTaskTrackEntry (struct)                [TaskTrack行——Title+Body+Task指针]
```

---

## 文件清单

| 文件 | 说明 |
|------|------|
| `Public/SSchedulerTrackTitle.h` + `.cpp` | BodyLeft 区单行标题 Widget |
| `Public/SSchedulerTrackBody.h` + `.cpp` | BodyRight 区单行体 Widget |
| `Public/SchedulerSubsystem.h` + `.cpp` | FTrack / FTaskTrackEntry + TrackMap + 增量管理 |
| `Public/USchedulerWidget.h` + `.cpp` | 蓝图属性 + ScrollBox 容器 + 滚动同步 |

---

## 控件层级

### SSchedulerTrackTitle

```
SButton("NoBorder", ContentPadding=0)
  └─ SBox(HeightOverride=RowHeight)
       └─ SOverlay
            ├─ SImage(描边色, DrawAs=Border, Margin=TitleMargin)
            ├─ SImage(底色, FSlateColorBrush(White), Padding=TitleMargin)
            └─ STextBlock(ColorAndOpacity=TextColor)
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

### DeleteTask

```
DeleteTask(Task)
  ├─ TaskMap[OwnerName].Remove(Task)
  ├─ 匹配 FTaskTrackEntry.Task → DestroyTaskTrackWidgets → RemoveSlot
  └─ OwnerTasks 为空?
       └─ DestroyOwnerTrackWidgets → RemoveSlot → TrackMap.Remove
```

### 折叠/展开

```
点击 OwnerTrack Title（SButton::OnClicked）
  ├─ bIsCollapsed = !bIsCollapsed
  ├─ 遍历 TaskTracks → SetVisibility(Collapsed / Visible)
  └─ Title->SetDisplayName("▶" / "▼" + OwnerName)
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

## Phase 3 待实现

- Keyframe 圆形/菱形 SImage 渲染（水平对齐 Ruler，聚合算法）
- 左键选中切换颜色（单选）
- 右键删除（选中态下调用 Task::DeleteKeyframe）
- RefreshUI 联动 Keyframe 位置刷新

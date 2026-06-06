# Scheduler Plugin v1.0

**赋予任意 UObject 时间调度能力**——无需继承特定基类，实现 `ITaskInterface` 即可让任何对象拥有时间轴驱动的 Task。

## 核心概念

### Task 是抽象能力，不是组件

```
任何 UObject 子类 + 实现 ITaskInterface = 拥有 Task 调度能力
```

不依赖 Actor、不依赖 Component、不依赖 Tick。Task 通过 `NewObject<USchedulerTask>(Owner)` 创建，Outer 即 Owner，GC 生命周期自动跟随。

### Tick = 纯粹的时间计量单位

不绑定帧、不绑定秒，只是整数刻度。`SetCurrentTime(100)` 代表时刻 100——与帧率无关，与秒无关。由你定义 Tick 的物理意义。

### 关键帧驱动

```
时刻线:  0───5───10──────20────30
         ●         ●            ●
      Keyframe   Keyframe    Keyframe
```

Task 持有有序关键帧数组。时间推进到关键帧时，**二分查找定位区间**，通过 `ITaskInterface::ExecuteTask(Time, Direction, ClipIndex)` 通知 Owner。Owner 决定"在这一帧做什么"。

## 功能

### Task 生命周期

| 操作 | 说明 |
|------|------|
| `CreateTask(Name, OwnerName, Owner)` | 创建 Task，Owner 自动获得调度能力 |
| `DestroyTask(Task)` | 销毁 Task + GC 标记，接口通知 Owner |
| `AddKeyframe(Tick, Array, Index, Inserted)` | 二分插入关键帧，去重 |
| `RemoveKeyframe(Tick, Index)` | 移除关键帧，接口通知 Owner |

### Track 可视化

| 功能 | 说明 |
|------|------|
| OwnerTrack | 按 OwnerName 分组，▼/► 折叠展开子 Task |
| TaskTrack | 缩进显示，Keyframe 圆点与刻度轴对齐 |
| Keyframe 交互 | 左键选中（蓝色高亮）/ 右键删除 |
| Playhead | 红色竖线 + 时间标签，跟随 CurrentTime |
| 增量同步 | CreateTask/DestroyTask 即时增删 Track，不重建 |

### 蓝图配置

所有视觉参数通过 `USchedulerWidget` 蓝图编辑：Track 颜色/字号/边框、Keyframe 尺寸/颜色/贴图、Playhead 宽度/颜色。

## ITaskInterface（3 个蓝图事件）

```cpp
UFUNCTION(BlueprintNativeEvent)
void ExecuteTask(int64 NewCurrentTime, bool bIsForward, FClipIndex ClipIndex);

UFUNCTION(BlueprintNativeEvent)
void DestroyTask();

UFUNCTION(BlueprintNativeEvent)
void RemoveKeyframe(int32 KeyframeIndex);
```

**任何一个 Blueprint 实现这 3 个函数，就能被 CreateTask 赋予调度能力。**

## 解耦设计

```
USchedulerTask（数据）          ←→  USchedulerSubsystem（调度器）
       │                                    │
       ▼                                    ▼
ITaskInterface（能力）             TrackMap（UI 池）
       │                                    │
       ▼                                    ▼
  任意 UObject                       SSchedulerTrack*
  (蓝图实现接口)                    (Slate 控件)
```

- Subsystem 只管时刻 + 池管理，不碰 UI 布局
- Task 只管关键帧数据 + 二分查找，不知道谁在显示它
- Widget 只管 Slate 渲染，所有属性 UPROPERTY 暴露
- **三者互相不知道对方存在**

## 快速开始

1. 创建 Widget 蓝图，放入 `USchedulerWidget`
2. 创建任意 Actor/Obj 蓝图，实现 `ITaskInterface`
3. 蓝图调用 `Subsystem->CreateTask("MyTask", "OwnerA", Self)` ——该对象立即拥有 Task 调度能力
4. 添加 Keyframe，观察 Track 面板上的圆点标记
5. `SetCurrentTime(100)` 驱动时刻前进，Owner 收到 `ExecuteTask` 回调

## 文档

- [TRACKPANEL_DSL.md](TRACKPANEL_DSL.md) — Track 面板架构设计
- [SCHEDULER_DSL.md](SCHEDULER_DSL.md) — 内核接口与委托链

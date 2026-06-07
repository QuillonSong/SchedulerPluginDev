# Scheduler Plugin v1.0

**一个 Runtime 调度器——让任意使用者在打包后的游戏中仍能进行时间轴编辑。**

## 设计初衷

Scheduler 的诞生源于一个简单的需求：**在 Runtime 环境下进行类时间轴编辑操作**。

问题在于，不同用户想要编辑的对象和逻辑完全是未知的——有人用它控制动画混合，有人用它驱动 Gameplay 技能，有人用它编排过场对话。插件不可能预知"被编辑的东西"会是什么。

所以 Scheduler 选择了一条路：**什么都不预设，只提供最原始的调度能力。** 它不是一个功能完备的时间轴编辑器，而是一个调度器——刻度计量、Task 承载、关键帧通知，仅此而已。剩下的一切，由你定义。

## 核心概念

### Tick —— 纯粹的计量单位

Tick 是 Scheduler 的最小刻度单位。它**没有物理定义**：不绑定秒、不绑定帧、不绑定任何现实时间概念。

1 就是 1，2 就是 2。`SetCurrentTime(100)` 表示时刻 100——至于 100 代表 100 秒、100 帧还是 100 个自定义步进，完全由你决定。

> 所有涉及 Tick 计数的变量统一使用 `int64`。

### Task —— 可赋予的能力

Task 继承自 `UObject`，是 Scheduler 对"用户自定义编辑行为"的抽象封装。

它的设计灵感来自 **Actor 与 Component 的关系**：你将 Component 挂载到 Actor 上，Actor 就获得了 Component 提供的能力。Task 遵循同样的思想——但更底层、更原始。它不与 Actor 耦合，不与 Component 耦合，不与任何特定类型耦合。

```
UObject 子类 + 实现 ITaskInterface = 拥有 Task 调度能力
```

- 你可以让 **Actor** 直接持有 Task
- 你可以让 **Component** 创建 Task，从而让 Actor 间接获得能力
- 你可以让 **UI（UUserWidget）** 或任意自定义 **UObject** 创建 Task

Task 不关心你是谁。你创建它，你就是它的 Owner。

### Track —— Task 的 UI 表现

创建 Task 后，Scheduler 会在 `SchedulerWidget` 中自动为其生成一个 **Track**（轨道行）。Track 按 OwnerName 分组——同一 Owner 下的所有 Task 归入一个可折叠展开的 OwnerTrack。

Track 就是 Task 在界面上的可视化锚点。添加关键帧后，它们会以圆点标记的形式出现在 Track 上，与刻度轴水平对齐。

### Keyframe + Playhead + Alpha

向 Task 添加 Keyframe（关键帧）后，它们会按 Tick 值有序排列在 Track 上。

拖动 **Playhead**（红色游标竖线），当它经过 Keyframe 区间时，TaskOwner 会通过 `ITaskInterface::ExecuteTask` 收到通知：

| 参数 | 含义 |
|------|------|
| `Alpha` | 当前时刻在两个 Keyframe 之间的归一化进度（0.0 ~ 1.0） |
| `bIsForward` | 时刻前进/后退方向 |
| `ClipIndex` | 所在区间的 LastIndex / NextIndex |

你不需要关心 Playhead 是怎么画的、Keyframe 是怎么渲染的。你只需要在 `ExecuteTask` 中根据 Alpha 和方向决定——**"在当前时刻，我的对象应该做什么。"**

### 外部数据维护

Scheduler **有意不内置保存/加载机制**。Task 的关键帧数据由你自行管理。

这不是偷懒，而是刻意选择：Runtime 时间轴编辑的需求千差万别，有人需要序列化到 SaveGame，有人需要网络同步到服务器，有人只需要内存中的临时编辑。任何内置方案都终将成为某些场景的负担。

**Scheduler 提供调度能力，你维护数据。** 我们相信这种解耦是最大的尊重。

## USchedulerWidget —— 一切编辑操作的容器

把 `USchedulerWidget` 从 UMG Palette 的 **Scheduler** 分类下拖入你的 Widget 蓝图——整个 Scheduler 的 UI 都在这里了。

### Head 区（上半区）

```
┌─────────────────────────────────────────────┐
│  HeadLeft（自定义面板）  │  HeadRight（刻度轴）  │
│  Customize 属性指定      │  SSchedulerRuler     │
│  UUserWidget 子类        │                       │
└─────────────────────────────────────────────┘
```

**HeadLeft** 是留给你的自定义面板。在 `USchedulerWidget` 的属性中找到 `Customize`，填入你创建的 `UUserWidget` 子类——运行时会自动创建实例挂入此位。播放、快进、暂停、后退……放你想放的任何控件。

**HeadRight** 是刻度轴（Ruler）。它由 `RulerTickLevel` 数组驱动：

| 属性 | 说明 |
|------|------|
| `Minor` | 短刻度线每格 = ? Tick |
| `Major` | 长刻度线每格 = ? Tick |
| `RulerMajorLength` | 长刻度线像素高度 |
| `RulerMinorLength` | 短刻度线像素高度 |

你可以添加多个层级——**滚轮滚动即可切换层级，这就是缩放。** 按住右键拖动刻度轴平移视野，按住左键拖动则持续更新当前时刻。

### Body 区（下半区）

```
┌─────────────────────────────────────────────┐
│  BodyLeft（Track 名称）  │  BodyRight（Keyframe）  │
│  SSchedulerTrackTitle    │  SSchedulerTrackBody   │
└─────────────────────────────────────────────┘
```

Body 区是 Track 的展示区域。Track 分两种：

| 类型 | 说明 |
|------|------|
| **OwnerTrack** | 按 OwnerName 分组，点击可折叠/展开其下所有 TaskTrack |
| **TaskTrack** | 缩进显示，属于某个 OwnerTrack 的子行 |

**OwnerTrack 右侧的 ✖** 会删除该 Owner 下所有 TaskTrack，同时销毁全部 Task。**TaskTrack 右侧的 ✖** 只删除那一个 Task。别担心数据丢失——Task 被销毁时 `DestroyTask` 事件会通知 Owner，你可以自行决定数据的去留。

### Keyframe 交互

在 Track 上：
- **左键点击** Keyframe → 选中（蓝色高亮）
- **右键双击** 已选中的 Keyframe → 删除，触发 `RemoveKeyframe(KeyframeIndex)` 通知

收到 `RemoveKeyframe` 后，根据返回的索引更新你的数据数组——这就是 `UTaskInterface` 的全部三个事件。

### Playhead —— 时间游标

那条贯穿 HeadRight + BodyRight 的红色长竖线就是 Playhead。点击刻度轴的某个刻度，它会跳过去；调用 `SetCurrentTime`，它也会跳过去。它上面的数字标签告诉你现在处于什么时刻。

### 蓝图节点

| 节点 | 用途 |
|------|------|
| `CreateTask(Name, OwnerName, Owner)` | 创建 Task——**把返回值存为变量**，后续 AddKeyframe 靠它 |
| `DestroyTask(Task)` | 销毁 Task，记得把变量置空 |
| `AddKeyframe(Tick, ...)` | 向 Task 添加关键帧 |
| `SetCurrentTime(Tick)` | 跳转到指定时刻 |
| `GetCurrentTime()` | 获取当前时刻 |
| `CurrentTimePlusPlus()` | 前进一个时刻 |
| `CurrentTimeMinusMinus()` | 后退一个时刻（触底 0 自动停止） |

### UI 自定义

`USchedulerWidget` 的 Details 面板提供了大量样式属性：Track 颜色/字号/边框、Keyframe 尺寸/颜色/贴图、Playhead 宽度/颜色、刻度轴样式……你可以根据变量名逐一调整。**修改后点击 UMG 编辑器左上角的编译按钮即可生效。**

## ITaskInterface

实现这个接口，你的对象就获得了 Task 调度能力。蓝图可直接实现。

```cpp
UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
void ExecuteTask(double Alpha, bool bIsForward, FClipIndex ClipIndex);
// 时刻变更通知——Alpha 为区间归一化进度 0.0~1.0

UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
void DestroyTask();
// Task 销毁通知

UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
void RemoveKeyframe(int32 KeyframeIndex);
// 关键帧移除通知
```

## 架构

```
USchedulerTask（数据）          ←→  USchedulerSubsystem（调度器）
       │                                    │
       ▼                                    ▼
ITaskInterface（能力）              TrackMap（UI 池）
       │                                    │
       ▼                                    ▼
  任意 UObject                       SSchedulerTrack*
  (蓝图实现接口)                    (Slate 控件)
```

- **Subsystem**：时刻调度 + Task/Track 池管理，不触碰 UI 布局
- **Task**：关键帧数据 + 二分查找，不知道谁在显示它
- **Widget**：Slate 渲染 + 蓝图属性暴露，不关心业务逻辑
- **Interface**：Task → Owner 的唯一通知通道

三者互相不知道对方存在。

## 功能

| 操作 | 说明 |
|------|------|
| `CreateTask(Name, OwnerName, Owner)` | NewObject 创建 Task，Owner 获得调度能力 |
| `DestroyTask(Task)` | 销毁 Task + UI + GC 标记，接口通知 Owner |
| `AddKeyframe(Tick, ...)` | 二分插入关键帧，去重 |
| `RemoveKeyframe(Tick, ...)` | 移除关键帧，接口通知 Owner |
| OwnerTrack 折叠/展开 | ▼/► 箭头 + 子 Task 显隐切换 |
| Keyframe 交互 | 左键选中（蓝色高亮）/ 右键删除 |
| Playhead | 红色竖线 + 时间标签，跟随 CurrentTime |
| 滚动同步 | Title/Body 两侧垂直滚动双向同步 |

## 快速开始

1. 创建 Widget 蓝图，放入 `USchedulerWidget`
2. 创建任意 Actor/Object/Widget 蓝图，实现 `ITaskInterface`
3. 调用 `Subsystem->CreateTask("MyTask", "OwnerA", Self)`——该对象立即拥有 Task
4. 添加 Keyframe，Track 面板上出现圆点标记
5. 拖动 Playhead 或调用 `SetCurrentTime`，Owner 收到 `ExecuteTask(Alpha, Direction, ClipIndex)`

## 打包

| 目标 | 状态 |
|------|------|
| UnrealEditor | ✅ |
| UnrealGame | ✅ |

插件为纯 Runtime 类型，无 Editor 模块依赖。可随游戏打包分发。

## 文档

- [SCHEDULER_DSL.md](SCHEDULER_DSL.md) — 内核架构、委托链、接口清单
- [TRACKPANEL_DSL.md](TRACKPANEL_DSL.md) — Track 面板、Keyframe 渲染、Playhead 设计

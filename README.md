# Scheduler Plugin v1.0

UE5 时间轴调度插件——抽象 Tick 时间单位，Task 关键帧驱动，可视化 Track 编辑。

## 架构

```
USchedulerSubsystem (WorldSubsystem)     ← 内核：Tick 时刻 + Task 池 + Track 池
USchedulerTask (UObject)                  ← 数据：关键帧数组 + 时刻回调
USchedulerWidget (UWidget)                ← UI 总容器（蓝图暴露口）
  ├─ SSchedulerWidget (Slate)             ← 十字四分割布局
  ├─ SSchedulerRuler (Slate)              ← 水平刻度尺（滚动/缩放/点击）
  ├─ SSchedulerTrackTitle (Slate)         ← Track 行标题（箭头/文字/删除）
  ├─ SSchedulerTrackBody (Slate)          ← Track 行体（Keyframe 渲染）
  └─ SSchedulerPlayhead (Slate)           ← 游标线（CurrentTime 指示）
```

## 功能

| 功能 | 说明 |
|------|------|
| Tick 时刻控制 | SetCurrentTime / CurrentTime++ / CurrentTime-- |
| Task 生命周期 | CreateTask → OnTaskInitialized / DestroyTask → OnDestroy |
| Keyframe 编辑 | AddKeyframe / RemoveKeyframe，Ruler 对齐渲染 |
| Keyframe 交互 | 左键选中（CheckedColor）/ 右键删除 |
| OwnerTrack 折叠 | ▼/► 箭头切换，TaskTrack 折叠/展开 |
| 增量 Track 管理 | TrackMap 池，CreateTask/DestroyTask 同步增删 |
| Playhead 游标 | 竖线 + 时间标签，OnTimeChanged 驱动，可见范围自适应 |
| 蓝图配置 | 全部视觉参数 UPROPERTY 暴露 |

## 蓝图接口

### USchedulerSubsystem

| 函数 | 说明 |
|------|------|
| `CreateTask(Name, OwnerName, Owner)` | 创建 Task |
| `DestroyTask(Task)` | 销毁 Task |
| `SetCurrentTime(Tick)` | 跳转时刻 |
| `GetCurrentTime()` | 获取时刻 |
| `initializationSubsystem()` | 归零 |

### ITaskInterface（TaskOwner 实现）

| 函数 | 说明 |
|------|------|
| `ExecuteTask(Time, Forward, ClipIndex)` | 时刻变更通知 |
| `DestroyTask()` | Task 销毁通知 |
| `RemoveKeyframe(Index)` | Keyframe 移除通知 |

### USchedulerWidget 属性

#### Layout
| 属性 | 默认值 |
|------|--------|
| HeadHeight | 100 |
| LeftSidebarWidth | 200 |
| bIsDrawCross | true |

#### Track
| 属性 | 默认值 |
|------|--------|
| TrackHeight | 24 |
| OwnerTrackColor | (0.08, 0.08, 0.08) |
| TaskTrackColor | (0.12, 0.12, 0.12) |
| TrackTextColor | (0.9, 0.9, 0.9) |
| TrackBorderColor | (0.2, 0.2, 0.2) |
| TrackTitleMargin | (1,1,1,1) |
| TrackBodyMargin | (1,1,0,1) |
| TrackFontSize | 10 |

#### Keyframe
| 属性 | 默认值 |
|------|--------|
| KeyframeSize | 10 |
| UnCheckedColor | White |
| CheckedColor | Blue |
| KeyframeTexture | nullptr |

#### Playhead
| 属性 | 默认值 |
|------|--------|
| PlayheadWidth | 2 |
| PlayheadColor | Red |
| PlayheadFontSize | 8 |
| PlayheadFontColor | Red |

## 文件

| 文件 | 说明 |
|------|------|
| `TRACKPANEL_DSL.md` | 架构 DSL（完整设计文档） |
| `SCHEDULER_DSL.md` | 内核 DSL（Subsystem/Task 接口） |

## 已知问题

- `[FIXME]` Keyframe 属性(Size/Color/Texture)修改后渲染不更新
- 聚合功能未实现
- 自定义画刷形(Circle/Triangle)未实现

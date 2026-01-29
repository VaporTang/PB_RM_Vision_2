# RM_Decision

## 1. 项目概述

机器人导航决策系统，根据裁判系统数据发布导航点，适用于RoboMaster比赛。

## 2. 功能特点

- 基于裁判系统数据（由rm_referee_ros2提供）进行导航决策
- 支持进攻策略和撤退策略
- 根据机器人血量和子弹余量自动切换策略
- 可配置的导航点和策略参数
- 灵活的策略执行流程

## 3. 策略类型

### 3.1 进攻策略

- 比赛开始或撤退策略结束后触发
- 包含最多4个导航点，每个点间有可配置的延迟
- 支持在第3、4点之间循环（若定义了第4点）或在第3点循环（若未定义第4点）

### 3.2 撤退策略

- 机器人血量低于阈值或子弹数量不足时触发
- 包含2个导航点，每个点间有可配置的延迟
- 完成后等待恢复时间，然后重新启动进攻策略

## 4. 配置文件

策略配置文件（默认为`config/strategies.yaml`）包含以下主要参数：

- **robot_id**: 机器人ID（默认为7，哨兵机器人）
- **hp_threshold**: 血量阈值，低于此值触发撤退
- **bullet_threshold**: 子弹阈值，低于此值触发撤退（每场比赛仅触发一次）
- **start_delay**: 比赛开始后延迟启动进攻策略的时间
- **offensive_strategies**: 进攻策略集合，可配置多个策略
- **defensive_strategies**: 撤退策略集合，可配置多个策略

## 5. 消息订阅与发布

### 5.1 订阅的话题

| 话题 | 消息类型 | 说明 |
|-----|---------|------|
| /referee/game_status | GameStatus | 比赛状态数据 |
| /referee/game_robot_hp | GameRobotHp | 机器人血量数据 |
| /referee/bullet_remaining | BulletRemaining | 子弹剩余数据 |

### 5.2 发布的话题

| 话题 | 消息类型 | 说明 |
|-----|---------|------|
| /goal_pose | geometry_msgs/PoseStamped | 导航目标点 |

## 6. 安装与使用

### 6.1 依赖项

- ROS2
- rm_referee_ros2包

### 6.2 编译

```bash
cd ~/ros2_ws
colcon build --packages-select rm_decision
source install/setup.bash
```

### 6.3 运行

使用launch文件启动：

```bash
ros2 launch rm_decision decision.launch.py
```

使用自定义配置文件：

```bash
ros2 launch rm_decision decision.launch.py config_path:=/path/to/your/config.yaml
```

## 7. 代码结构

### 7.1 主要类

| 类名 | 文件 | 功能 |
|------|------|------|
| DecisionSystem | decision_system.hpp/cpp | 主要决策系统，处理裁判系统数据并发布导航点 |
| StrategyManager | strategy_manager.hpp/cpp | 策略管理器，负责加载和管理不同的策略 |

### 7.2 核心文件

| 文件 | 说明 |
|------|------|
| decision_system.hpp | 定义决策系统类及其方法 |
| decision_system.cpp | 实现决策逻辑，包括策略切换和导航点发布 |
| strategy_manager.hpp | 定义策略管理器类及相关数据结构 |
| strategy_manager.cpp | 实现配置文件加载和策略管理功能 |
| decision_node.cpp | 主节点入口文件 |

## 8. 策略执行流程

1. 比赛开始，等待一段时间后启动进攻策略
2. 按顺序导航到各个点，每个点间有指定延迟
3. 到达第3点后，根据是否有第4点决定循环方式
4. 当触发撤退条件（血量低/子弹不足）时：
   - 如果当前执行第2点及以后的导航点，直接发布最后一个撤退点
   - 否则，发布第1个撤退点，然后发布第2个撤退点
5. 撤退完成后，等待恢复时间，然后重新启动进攻策略 
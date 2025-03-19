# Fake Serial ROS2

## 项目简介

Fake Serial ROS2是一个用于虚拟化rm_referee_ros2功能的工具。由于裁判系统串口设备不常能获得，该工具可以提供一个Qt界面，允许用户手动设置和发布所有裁判系统相关的ROS2话题，用于调试依赖这些数据的下游项目。

## 功能特点

- 支持所有rm_referee_ros2发布的话题
- 通过Qt GUI界面方便地修改和发布数据
- 支持自动定时发布数据
- 可设置所有裁判系统相关的字段
- 与rm_referee_ros2使用相同的话题名和消息类型，完全兼容

## 支持的消息类型

- 比赛状态数据 (GameStatus)
- 机器人血量数据 (GameRobotHp)
- 比赛结果数据 (GameResult)
- 场地事件数据 (FieldEvents)
- 机器人伤害数据 (RobotHurt)
- 裁判警告数据 (RefereeWarning)
- 射击数据 (ShootData)
- 子弹剩余数据 (BulletRemaining)
- 机器人状态数据 (RobotStatus)
- 机器人位置数据 (RobotPos)

## 编译和运行

### 编译

确保您的ROS2环境已经配置好，然后在工作空间根目录执行：

```bash
colcon build --packages-select fake_serial_ros2
```

### 运行

使用launch文件启动：

```bash
ros2 launch fake_serial_ros2 fake_serial.launch.py
```

或直接运行节点：

```bash
ros2 run fake_serial_ros2 fake_serial_node
```

## 使用方法

1. 启动应用后，会显示一个带有多个标签页的Qt界面
2. 每个标签页对应一种裁判系统消息类型
3. 在各标签页中设置所需的参数值
4. 点击对应标签页中的"发布"按钮，将数据发布到相应的ROS2话题
5. 可以勾选"自动发布"选项，并设置发布间隔，实现定时自动发布所有数据
6. 点击主界面底部的"发布所有数据"按钮，可一次性发布所有标签页的数据

## 话题列表

该工具发布的话题与rm_referee_ros2完全一致：

- `/referee/game_status`
- `/referee/game_robot_hp`
- `/referee/game_result`
- `/referee/field_events`
- `/referee/robot_hurt`
- `/referee/referee_warning`
- `/referee/shoot_data`
- `/referee/bullet_remaining`
- `/referee/robot_status`
- `/referee/robot_pos` 
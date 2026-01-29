# RM Decision TMP

这是一个ROS2决策包，用于在没有导航雷达情况下控制机器人行为。通过使用机器人的里程计和IMU数据，实现简单的导航行为。

## 功能

- 通过发布 `geometry_msgs/msg/Twist` 消息到 `/cmd_vel` 话题控制机器人运动
- 接收里程计 (`/odom`) 和IMU (`/imu`) 数据进行简单的状态感知
- 实现基本的行为状态机，可根据需要扩展
- 监听裁判系统比赛状态 (`/referee/game_status`)，在比赛开始后执行特定动作
- 支持在比赛开始后前进指定距离并自动停止

## 安装

克隆此代码仓库到ROS2工作空间的`src`目录，然后编译：

```bash
cd ~/your_workspace
colcon build --packages-select rm_decision_tmp
source install/setup.bash
```

## 运行

使用launch文件启动，可指定前进距离：

```bash
# 使用默认参数
ros2 launch rm_decision_tmp simple_navigation.launch.py

# 指定前进距离为2米
ros2 launch rm_decision_tmp simple_navigation.launch.py move_distance:=2.0
```

## 配置

可以通过两种方式配置节点参数：

1. 使用YAML配置文件（位于`config/navigation_params.yaml`）
2. 直接在launch文件中修改参数
3. 通过launch文件参数覆盖默认值

主要参数包括：

- `cmd_vel_topic`: 控制命令发布的话题
- `odom_topic`: 里程计数据订阅话题
- `imu_topic`: IMU数据订阅话题
- `game_status_topic`: 比赛状态话题
- `linear_speed`: 线速度（米/秒）
- `angular_speed`: 角速度（弧度/秒）
- `control_frequency`: 控制频率（赫兹）
- `move_distance`: 比赛开始后前进的距离（米）

## 运行逻辑

1. 节点启动后，默认处于停止状态，等待比赛开始信号
2. 当收到比赛开始信号（比赛状态值为4）时，机器人将开始前进指定距离
3. 使用里程计数据计算移动距离，当达到设定值时自动停止
4. 如果比赛中断或结束，机器人将立即停止移动

## 扩展

通过修改`src/simple_navigation.cpp`中的`control_loop()`函数，可以实现更复杂的导航策略。 
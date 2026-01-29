#include "rm_decision/decision_system.hpp"

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <string>
#include <functional>
#include <memory>
#include <iostream>

namespace rm_decision
{

using namespace std::chrono_literals;
using std::placeholders::_1;

DecisionSystem::DecisionSystem(const rclcpp::NodeOptions & node_options)
: Node("decision_system", node_options),
  current_nav_point_index_(0),
  strategy_in_progress_(false),
  game_started_(false),
  bullet_threshold_triggered_(false),
  current_robot_hp_(1000),  // 默认最大血量
  current_bullet_remaining_(0)
{
  // 声明并获取参数
  this->declare_parameter("config_path", "");
  this->declare_parameter("frame_id", "map");
  
  config_path_ = this->get_parameter("config_path").as_string();
  frame_id_ = this->get_parameter("frame_id").as_string();
  
  // 如果未指定配置文件路径，使用默认路径
  if (config_path_.empty()) {
    config_path_ = ament_index_cpp::get_package_share_directory("rm_decision") +
      "/config/strategies.yaml";
  }
  
  // 创建策略管理器
  strategy_manager_ = std::make_unique<StrategyManager>(config_path_);
  
  // 获取机器人ID和阈值
  robot_id_ = strategy_manager_->getRobotId();
  hp_threshold_ = strategy_manager_->getHpThreshold();
  bullet_threshold_ = strategy_manager_->getBulletThreshold();
  
  // 创建发布者
  nav_goal_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>(
    "/goal_pose", 10);
  
  // 创建订阅者
  game_status_sub_ = this->create_subscription<rm_referee_ros2::msg::GameStatus>(
    "/referee/game_status", 10, std::bind(&DecisionSystem::gameStatusCallback, this, _1));
    
  game_robot_hp_sub_ = this->create_subscription<rm_referee_ros2::msg::GameRobotHp>(
    "/referee/game_robot_hp", 10, std::bind(&DecisionSystem::gameRobotHpCallback, this, _1));
    
  bullet_remaining_sub_ = this->create_subscription<rm_referee_ros2::msg::BulletRemaining>(
    "/referee/bullet_remaining", 10, std::bind(&DecisionSystem::bulletRemainingCallback, this, _1));
  
  RCLCPP_INFO(this->get_logger(), "决策系统已初始化，配置文件: %s", config_path_.c_str());
  RCLCPP_INFO(this->get_logger(), "机器人ID: %d, 血量阈值: %d, 子弹阈值: %d", 
    robot_id_, hp_threshold_, bullet_threshold_);
}

void DecisionSystem::gameStatusCallback(const rm_referee_ros2::msg::GameStatus::SharedPtr msg)
{
  // 检查比赛是否开始
  if (!game_started_ && msg->game_progress == 4) {  // 4 = 比赛中
    game_started_ = true;
    RCLCPP_INFO(this->get_logger(), "比赛开始，将在%f秒后开始执行进攻策略", 
      strategy_manager_->getStartDelay());
    
    // 设置定时器，在指定延迟后开始进攻策略
    strategy_timer_ = this->create_wall_timer(
      std::chrono::duration<double>(strategy_manager_->getStartDelay()),
      std::bind(&DecisionSystem::startOffensiveStrategy, this));
  } else if (game_started_ && msg->game_progress != 4) {
    // 比赛结束或暂停
    game_started_ = false;
    strategy_in_progress_ = false;
    if (strategy_timer_) {
      strategy_timer_->cancel();
    }
    RCLCPP_INFO(this->get_logger(), "比赛已结束或暂停");
  }
}

void DecisionSystem::gameRobotHpCallback(const rm_referee_ros2::msg::GameRobotHp::SharedPtr msg)
{
  // 根据机器人ID获取当前血量
  uint16_t hp = 0;
  
  // 根据ID获取相应机器人的血量
  if (robot_id_ == 7) {  // 哨兵
    hp = (robot_id_ < 10) ? msg->red_7_robot_hp : msg->blue_7_robot_hp;
  } else if (robot_id_ == 1) {  // 英雄
    hp = (robot_id_ < 10) ? msg->red_1_robot_hp : msg->blue_1_robot_hp;
  } else if (robot_id_ == 2) {  // 工程
    hp = (robot_id_ < 10) ? msg->red_2_robot_hp : msg->blue_2_robot_hp;
  } else if (robot_id_ == 3) {  // 步兵3
    hp = (robot_id_ < 10) ? msg->red_3_robot_hp : msg->blue_3_robot_hp;
  } else if (robot_id_ == 4) {  // 步兵4
    hp = (robot_id_ < 10) ? msg->red_4_robot_hp : msg->blue_4_robot_hp;
  }
  
  current_robot_hp_ = hp;
  
  // 检查血量是否低于阈值，触发撤退策略
  if (game_started_ && strategy_in_progress_ && 
      current_strategy_type_ == StrategyType::OFFENSIVE &&
      hp > 0 && hp < hp_threshold_) {
    RCLCPP_INFO(this->get_logger(), "机器人血量低于阈值(%d < %d)，触发撤退策略", 
      hp, hp_threshold_);
    startDefensiveStrategy();
  }
}

void DecisionSystem::bulletRemainingCallback(const rm_referee_ros2::msg::BulletRemaining::SharedPtr msg)
{
  // 记录剩余子弹数
  current_bullet_remaining_ = msg->bullet_allowance_num_17_mm;  // 假设使用17mm子弹
  
  // 检查是否低于阈值，且之前未触发过此条件
  if (game_started_ && strategy_in_progress_ && 
      current_strategy_type_ == StrategyType::OFFENSIVE &&
      !bullet_threshold_triggered_ && 
      current_bullet_remaining_ < bullet_threshold_) {
    
    RCLCPP_INFO(this->get_logger(), "剩余子弹低于阈值(%d < %d)，触发撤退策略", 
      current_bullet_remaining_, bullet_threshold_);
    
    bullet_threshold_triggered_ = true;  // 标记已触发
    startDefensiveStrategy();
  }
}

void DecisionSystem::startOffensiveStrategy()
{
  // 取消已有定时器
  if (strategy_timer_) {
    strategy_timer_->cancel();
  }
  
  // 加载进攻策略
  current_strategy_ = strategy_manager_->getStrategy(StrategyType::OFFENSIVE);
  current_strategy_type_ = StrategyType::OFFENSIVE;
  current_nav_point_index_ = 0;
  strategy_in_progress_ = true;
  
  RCLCPP_INFO(this->get_logger(), "开始执行进攻策略: %s", current_strategy_.name.c_str());
  
  // 发布第一个导航点
  if (!current_strategy_.nav_points.empty()) {
    publishNavPoint(current_strategy_.nav_points[0]);
    scheduleNextPoint(current_strategy_.nav_points[0].delay);
  } else {
    RCLCPP_ERROR(this->get_logger(), "进攻策略中没有导航点");
    strategy_in_progress_ = false;
  }
}

void DecisionSystem::startDefensiveStrategy()
{
  // 取消已有定时器
  if (strategy_timer_) {
    strategy_timer_->cancel();
  }
  
  // 加载撤退策略
  current_strategy_ = strategy_manager_->getStrategy(StrategyType::DEFENSIVE);
  current_strategy_type_ = StrategyType::DEFENSIVE;
  strategy_in_progress_ = true;
  
  RCLCPP_INFO(this->get_logger(), "开始执行撤退策略: %s", current_strategy_.name.c_str());
  
  // 根据当前进攻策略的执行情况，决定发布哪个撤退点
  if (current_strategy_.nav_points.size() >= 2) {
    if (current_nav_point_index_ >= 1) {
      // 如果正在执行第二个或更后面的进攻点，直接执行最后一个撤退点
      current_nav_point_index_ = 1;
      publishNavPoint(current_strategy_.nav_points[1]);
      
      // 撤退完成后，恢复进攻策略
      strategy_timer_ = this->create_wall_timer(
        std::chrono::duration<double>(current_strategy_.recovery_delay),
        std::bind(&DecisionSystem::startOffensiveStrategy, this));
    } else {
      // 否则从第一个撤退点开始
      current_nav_point_index_ = 0;
      publishNavPoint(current_strategy_.nav_points[0]);
      
      // 安排发布第二个点
      strategy_timer_ = this->create_wall_timer(
        std::chrono::duration<double>(current_strategy_.nav_points[0].delay),
        [this]() {
          current_nav_point_index_ = 1;
          publishNavPoint(current_strategy_.nav_points[1]);
          
          // 撤退完成后，恢复进攻策略
          strategy_timer_ = this->create_wall_timer(
            std::chrono::duration<double>(current_strategy_.recovery_delay),
            std::bind(&DecisionSystem::startOffensiveStrategy, this));
        });
    }
  } else {
    RCLCPP_ERROR(this->get_logger(), "撤退策略中没有足够的导航点");
    strategy_in_progress_ = false;
  }
}

void DecisionSystem::strategyTimerCallback()
{
  if (!strategy_in_progress_ || !game_started_) {
    return;
  }
  
  if (current_strategy_type_ == StrategyType::OFFENSIVE) {
    // 进攻策略逻辑
    // 移动到下一个导航点
    current_nav_point_index_++;
    
    auto & nav_points = current_strategy_.nav_points;
    size_t nav_points_size = nav_points.size();
    
    if (current_nav_point_index_ < nav_points_size) {
      // 发布下一个导航点
      publishNavPoint(nav_points[current_nav_point_index_]);
      
      // 安排下一个点
      scheduleNextPoint(nav_points[current_nav_point_index_].delay);
    } else if (nav_points_size >= 3) {
      // 进入循环模式
      if (nav_points_size == 3) {
        // 如果只有3个点，在第3点循环
        current_nav_point_index_ = 2;
        publishNavPoint(nav_points[2]);
        
        // 循环延迟
        scheduleNextPoint(current_strategy_.loop_delay);
      } else {
        // 如果有4个点，在第3和第4点之间循环
        if (current_nav_point_index_ >= 4) {
          // 从第4点返回第3点
          current_nav_point_index_ = 2;
          publishNavPoint(nav_points[2]);
        } else {
          // 从第3点到第4点
          current_nav_point_index_ = 3;
          publishNavPoint(nav_points[3]);
        }
        
        // 循环延迟
        scheduleNextPoint(current_strategy_.loop_delay);
      }
    }
  }
}

void DecisionSystem::publishNavPoint(const NavPoint & nav_point)
{
  // 复制导航点并设置帧ID和时间戳
  geometry_msgs::msg::PoseStamped pose = nav_point.pose;
  pose.header.frame_id = frame_id_;
  pose.header.stamp = this->now();
  
  // 发布导航点
  nav_goal_pub_->publish(pose);
  
  RCLCPP_INFO(this->get_logger(), "发布导航点 [%d]: (%.2f, %.2f), 延迟: %.2f秒",
    static_cast<int>(current_nav_point_index_) + 1,
    pose.pose.position.x, pose.pose.position.y,
    nav_point.delay);
}

void DecisionSystem::scheduleNextPoint(double delay_seconds)
{
  // 取消已有定时器
  if (strategy_timer_) {
    strategy_timer_->cancel();
  }
  
  // 创建新定时器
  strategy_timer_ = this->create_wall_timer(
    std::chrono::duration<double>(delay_seconds),
    std::bind(&DecisionSystem::strategyTimerCallback, this));
    
  RCLCPP_INFO(this->get_logger(), "计划在%.2f秒后发布下一个导航点", delay_seconds);
}

}  // namespace rm_decision 
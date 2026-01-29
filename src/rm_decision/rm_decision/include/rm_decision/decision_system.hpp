#ifndef RM_DECISION_DECISION_SYSTEM_HPP_
#define RM_DECISION_DECISION_SYSTEM_HPP_

#include <memory>
#include <string>
#include <vector>
#include <chrono>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "rm_referee_ros2/msg/game_status.hpp"
#include "rm_referee_ros2/msg/game_robot_hp.hpp"
#include "rm_referee_ros2/msg/bullet_remaining.hpp"
#include "rm_decision/strategy_manager.hpp"

namespace rm_decision
{

/**
 * @brief 决策系统类，负责根据裁判系统数据制定和执行导航策略
 */
class DecisionSystem : public rclcpp::Node
{
public:
  /**
   * @brief 构造函数
   * @param node_options 节点选项
   */
  explicit DecisionSystem(const rclcpp::NodeOptions & node_options = rclcpp::NodeOptions());
  
  /**
   * @brief 析构函数
   */
  virtual ~DecisionSystem() = default;

private:
  // 订阅回调函数
  void gameStatusCallback(const rm_referee_ros2::msg::GameStatus::SharedPtr msg);
  void gameRobotHpCallback(const rm_referee_ros2::msg::GameRobotHp::SharedPtr msg);
  void bulletRemainingCallback(const rm_referee_ros2::msg::BulletRemaining::SharedPtr msg);
  
  // 定时器回调函数
  void strategyTimerCallback();
  
  // 启动进攻策略
  void startOffensiveStrategy();
  
  // 启动撤退策略
  void startDefensiveStrategy();
  
  // 发布导航点
  void publishNavPoint(const NavPoint & nav_point);
  
  // 创建定时器以便在一定时间后发布下一个导航点
  void scheduleNextPoint(double delay_seconds);
  
  // 订阅者
  rclcpp::Subscription<rm_referee_ros2::msg::GameStatus>::SharedPtr game_status_sub_;
  rclcpp::Subscription<rm_referee_ros2::msg::GameRobotHp>::SharedPtr game_robot_hp_sub_;
  rclcpp::Subscription<rm_referee_ros2::msg::BulletRemaining>::SharedPtr bullet_remaining_sub_;
  
  // 发布者
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr nav_goal_pub_;
  
  // 定时器
  rclcpp::TimerBase::SharedPtr strategy_timer_;
  
  // 策略管理器
  std::unique_ptr<StrategyManager> strategy_manager_;
  
  // 当前策略相关状态
  Strategy current_strategy_;
  StrategyType current_strategy_type_;
  size_t current_nav_point_index_;
  bool strategy_in_progress_;
  bool game_started_;
  bool bullet_threshold_triggered_;
  
  // 记录当前机器人状态
  uint16_t current_robot_hp_;
  uint16_t current_bullet_remaining_;
  
  // 机器人ID和相关阈值
  int robot_id_;
  uint16_t hp_threshold_;
  uint16_t bullet_threshold_;
  
  // 参数
  std::string config_path_;
  std::string frame_id_;
};

}  // namespace rm_decision

#endif  // RM_DECISION_DECISION_SYSTEM_HPP_ 
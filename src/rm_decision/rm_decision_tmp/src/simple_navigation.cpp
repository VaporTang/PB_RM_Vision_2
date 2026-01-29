#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "rm_referee_ros2/msg/game_status.hpp"

using namespace std::chrono_literals;

/**
 * @brief 简单导航节点，在没有雷达、IMU和里程计情况下进行机器人控制
 */
class SimpleNavigationNode : public rclcpp::Node
{
public:
  SimpleNavigationNode()
  : Node("simple_navigation_node")
  {
    // 声明参数
    this->declare_parameter("cmd_vel_topic", "/cmd_vel_chassis");
    this->declare_parameter("game_status_topic", "/referee/game_status");
    this->declare_parameter("linear_speed", 0.2);
    this->declare_parameter("control_frequency", 10.0);
    this->declare_parameter("move_distance", 1.0);  // 添加前进距离参数，单位：米
    this->declare_parameter("move_time_factor", 5.0);  // 添加时间因子参数，默认为5秒/米

    // 获取参数
    std::string cmd_vel_topic = this->get_parameter("cmd_vel_topic").as_string();
    std::string game_status_topic = this->get_parameter("game_status_topic").as_string();
    linear_speed_ = this->get_parameter("linear_speed").as_double();
    move_distance_ = this->get_parameter("move_distance").as_double();
    move_time_factor_ = this->get_parameter("move_time_factor").as_double();
    double control_frequency = this->get_parameter("control_frequency").as_double();
    
    // 计算需要移动的时间（秒）
    move_time_seconds_ = move_distance_ * move_time_factor_;

    // 创建发布者和订阅者
    cmd_vel_publisher_ = this->create_publisher<geometry_msgs::msg::Twist>(
      cmd_vel_topic, 10);

    // 比赛状态订阅
    game_status_subscription_ = this->create_subscription<rm_referee_ros2::msg::GameStatus>(
      game_status_topic, 10, 
      std::bind(&SimpleNavigationNode::game_status_callback, this, std::placeholders::_1));

    // 创建定时器
    auto period = std::chrono::duration<double>(1.0 / control_frequency);
    timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::milliseconds>(period),
      std::bind(&SimpleNavigationNode::control_loop, this));

    RCLCPP_INFO(this->get_logger(), "简单导航节点已初始化");
    RCLCPP_INFO(this->get_logger(), "发布到 %s 话题", cmd_vel_topic.c_str());
    RCLCPP_INFO(this->get_logger(), "监听比赛状态 %s 话题", game_status_topic.c_str());
    RCLCPP_INFO(this->get_logger(), "设置前进距离为 %.2f 米, 预计需要 %.2f 秒", 
                move_distance_, move_time_seconds_);
  }

private:
  // 比赛状态回调函数
  void game_status_callback(const rm_referee_ros2::msg::GameStatus::SharedPtr msg)
  {
    // 直接使用消息中的game_progress字段
    uint8_t progress = msg->game_progress;
    
    // 检查比赛是否开始
    if (progress == 4) {  // 4表示比赛中
      if (!game_started_) {
        RCLCPP_INFO(this->get_logger(), "比赛开始，准备前进指定距离");
        game_started_ = true;
        
        // 记录开始时间，用于估计移动距离
        move_start_time_ = this->now();
        
        // 切换到前进指定距离状态
        current_state_ = State::MOVING_SPECIFIC_DISTANCE;
      }
    } else if (game_started_ && progress != 4) {
      // 比赛结束或暂停
      RCLCPP_INFO(this->get_logger(), "比赛已停止或暂停");
      game_started_ = false;
      current_state_ = State::STOPPED;
    }
    
    RCLCPP_DEBUG(this->get_logger(), "收到比赛状态数据: 阶段=%d, 比赛类型=%d, 剩余时间=%d秒", 
                 msg->game_progress, msg->game_type, msg->stage_remain_time);
  }

  // 控制循环，根据状态发布速度指令
  void control_loop()
  {
    // 创建速度消息
    auto twist_msg = std::make_unique<geometry_msgs::msg::Twist>();
    
    // 根据当前状态执行不同的控制逻辑
    switch (current_state_) {
      case State::STOPPED:
        // 停止状态，不发布速度指令
        twist_msg->linear.x = 0.0;
        twist_msg->angular.z = 0.0;
        break;
        
      case State::MOVING_SPECIFIC_DISTANCE:
        // 前进指定距离状态（基于时间估计）
        {
          // 计算已运行的时间
          rclcpp::Time current_time = this->now();
          double elapsed_seconds = (current_time - move_start_time_).seconds();
          
          // 估计已移动的距离
          double estimated_distance = linear_speed_ * elapsed_seconds;
          
          if (elapsed_seconds < move_time_seconds_) {
            // 未达到目标时间，继续前进
            twist_msg->linear.x = linear_speed_;
            twist_msg->angular.z = 0.0;
            RCLCPP_DEBUG(this->get_logger(), "前进中：已运行 %.2f/%.2f 秒, 估计距离 %.2f/%.2f 米", 
                        elapsed_seconds, move_time_seconds_, estimated_distance, move_distance_);
          } else {
            // 达到目标时间，停止并切换状态
            twist_msg->linear.x = 0.0;
            twist_msg->angular.z = 0.0;
            current_state_ = State::STOPPED;
            RCLCPP_INFO(this->get_logger(), "已完成前进任务, 运行时间 %.2f 秒, 估计距离 %.2f 米", 
                        elapsed_seconds, estimated_distance);
          }
        }
        break;
        
      default:
        // 未知状态，停止移动
        twist_msg->linear.x = 0.0;
        twist_msg->linear.y = 0.0;
        twist_msg->linear.z = 0.0;
        twist_msg->angular.x = 0.0;
        twist_msg->angular.y = 0.0;
        twist_msg->angular.z = 0.0;
        break;
    }
    
    // 发布速度指令
    cmd_vel_publisher_->publish(std::move(twist_msg));
  }

  // 机器人状态枚举
  enum class State {
    STOPPED,               // 停止状态
    MOVING_SPECIFIC_DISTANCE // 前进指定距离状态
  };

  // 发布者和订阅者
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_publisher_;
  rclcpp::Subscription<rm_referee_ros2::msg::GameStatus>::SharedPtr game_status_subscription_;
  
  // 定时器
  rclcpp::TimerBase::SharedPtr timer_;
  
  // 状态变量
  bool game_started_ = false;
  
  // 时间和距离控制
  rclcpp::Time move_start_time_;
  double move_distance_ = 1.0;  // 默认前进1米
  double move_time_seconds_ = 5.0;  // 默认移动时间5秒
  double move_time_factor_ = 5.0;  // 时间因子，默认5秒/米
  
  // 控制参数
  double linear_speed_;
  
  // 状态控制
  State current_state_ = State::STOPPED;  // 初始状态为停止
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<SimpleNavigationNode>());
  rclcpp::shutdown();
  return 0;
} 
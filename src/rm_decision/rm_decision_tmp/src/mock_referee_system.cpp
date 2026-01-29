#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "rm_referee_ros2/msg/game_status.hpp"
#include "rm_referee_ros2/msg/robot_pos.hpp"
#include "rm_referee_ros2/msg/robot_status.hpp"
#include "rm_referee_ros2/msg/bullet_remaining.hpp"
#include "builtin_interfaces/msg/time.hpp"

using namespace std::chrono_literals;

/**
 * @brief 模拟裁判系统节点，将nav2的定位数据转换为裁判系统格式
 */
class MockRefereeSystemNode : public rclcpp::Node
{
public:
  MockRefereeSystemNode() : Node("mock_referee_system_node")
  {
    // 声明参数
    this->declare_parameter("odom_topic", "/odom");
    this->declare_parameter("amcl_pose_topic", "/amcl_pose");
    this->declare_parameter("use_amcl", true);
    this->declare_parameter("publish_frequency", 10.0);
    this->declare_parameter("game_status_topic", "/referee/game_status");
    this->declare_parameter("robot_pos_topic", "/referee/robot_pos");
    this->declare_parameter("robot_status_topic", "/referee/robot_status");
    this->declare_parameter("bullet_remaining_topic", "/referee/bullet_remaining");
    this->declare_parameter("initial_hp", 500);
    this->declare_parameter("initial_bullets", 300);
    this->declare_parameter("robot_id", 1);

    // 获取参数
    std::string odom_topic = this->get_parameter("odom_topic").as_string();
    std::string amcl_pose_topic = this->get_parameter("amcl_pose_topic").as_string();
    bool use_amcl = this->get_parameter("use_amcl").as_bool();
    double publish_frequency = this->get_parameter("publish_frequency").as_double();
    game_status_topic_ = this->get_parameter("game_status_topic").as_string();
    robot_pos_topic_ = this->get_parameter("robot_pos_topic").as_string();
    robot_status_topic_ = this->get_parameter("robot_status_topic").as_string();
    bullet_remaining_topic_ = this->get_parameter("bullet_remaining_topic").as_string();
    initial_hp_ = this->get_parameter("initial_hp").as_int();
    initial_bullets_ = this->get_parameter("initial_bullets").as_int();
    robot_id_ = this->get_parameter("robot_id").as_int();

    // 初始化机器人状态
    current_hp_ = initial_hp_;
    current_bullets_ = initial_bullets_;
    game_started_ = false;

    // 创建发布者
    game_status_publisher_ = this->create_publisher<rm_referee_ros2::msg::GameStatus>(
      game_status_topic_, 10);
    robot_pos_publisher_ = this->create_publisher<rm_referee_ros2::msg::RobotPos>(
      robot_pos_topic_, 10);
    robot_status_publisher_ = this->create_publisher<rm_referee_ros2::msg::RobotStatus>(
      robot_status_topic_, 10);
    bullet_remaining_publisher_ = this->create_publisher<rm_referee_ros2::msg::BulletRemaining>(
      bullet_remaining_topic_, 10);

    // 创建订阅者（根据参数选择Odom或AMCL位姿）
    if (use_amcl) {
      amcl_pose_subscription_ = this->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
        amcl_pose_topic, 10, 
        std::bind(&MockRefereeSystemNode::amcl_pose_callback, this, std::placeholders::_1));
      RCLCPP_INFO(this->get_logger(), "使用AMCL定位数据，订阅话题: %s", amcl_pose_topic.c_str());
    } else {
      odom_subscription_ = this->create_subscription<nav_msgs::msg::Odometry>(
        odom_topic, 10, 
        std::bind(&MockRefereeSystemNode::odom_callback, this, std::placeholders::_1));
      RCLCPP_INFO(this->get_logger(), "使用里程计数据，订阅话题: %s", odom_topic.c_str());
    }

    // 创建定时器，定期发布裁判系统数据
    auto period = std::chrono::duration<double>(1.0 / publish_frequency);
    timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::milliseconds>(period),
      std::bind(&MockRefereeSystemNode::publish_referee_data, this));

    // 创建游戏开始定时器（5秒后开始游戏）
    game_start_timer_ = this->create_wall_timer(
      5s, std::bind(&MockRefereeSystemNode::start_game, this));

    RCLCPP_INFO(this->get_logger(), "模拟裁判系统节点已初始化");
    RCLCPP_INFO(this->get_logger(), "发布到以下话题:");
    RCLCPP_INFO(this->get_logger(), "  - 比赛状态: %s", game_status_topic_.c_str());
    RCLCPP_INFO(this->get_logger(), "  - 机器人位置: %s", robot_pos_topic_.c_str());
    RCLCPP_INFO(this->get_logger(), "  - 机器人状态: %s", robot_status_topic_.c_str());
    RCLCPP_INFO(this->get_logger(), "  - 子弹剩余: %s", bullet_remaining_topic_.c_str());
    RCLCPP_INFO(this->get_logger(), "初始血量: %d, 初始子弹: %d", initial_hp_, initial_bullets_);
  }

private:
  // AMCL位姿回调
  void amcl_pose_callback(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg)
  {
    current_x_ = msg->pose.pose.position.x;
    current_y_ = msg->pose.pose.position.y;
    
    // 从四元数计算yaw角度
    double qx = msg->pose.pose.orientation.x;
    double qy = msg->pose.pose.orientation.y;
    double qz = msg->pose.pose.orientation.z;
    double qw = msg->pose.pose.orientation.w;
    
    // 转换四元数到欧拉角
    double siny_cosp = 2.0 * (qw * qz + qx * qy);
    double cosy_cosp = 1.0 - 2.0 * (qy * qy + qz * qz);
    current_angle_ = std::atan2(siny_cosp, cosy_cosp) * 180.0 / M_PI;  // 转换为度
    
    has_pos_ = true;
    last_pose_time_ = this->now();
  }

  // 里程计回调
  void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    current_x_ = msg->pose.pose.position.x;
    current_y_ = msg->pose.pose.position.y;
    
    // 从四元数计算yaw角度
    double qx = msg->pose.pose.orientation.x;
    double qy = msg->pose.pose.orientation.y;
    double qz = msg->pose.pose.orientation.z;
    double qw = msg->pose.pose.orientation.w;
    
    // 转换四元数到欧拉角
    double siny_cosp = 2.0 * (qw * qz + qx * qy);
    double cosy_cosp = 1.0 - 2.0 * (qy * qy + qz * qz);
    current_angle_ = std::atan2(siny_cosp, cosy_cosp) * 180.0 / M_PI;  // 转换为度
    
    has_pos_ = true;
    last_pose_time_ = this->now();
  }

  // 开始游戏
  void start_game()
  {
    game_start_timer_.reset();  // 停止定时器，不再触发
    game_started_ = true;
    RCLCPP_INFO(this->get_logger(), "游戏开始！");
  }

  // 发布裁判系统数据
  void publish_referee_data()
  {
    // 检查位置数据是否过期（超过1秒未更新）
    auto current_time = this->now();
    bool pos_data_stale = (current_time - last_pose_time_).seconds() > 1.0;
    
    // 创建ROS 2标准时间戳
    builtin_interfaces::msg::Time ros_time;
    ros_time.sec = static_cast<int32_t>(current_time.seconds());
    ros_time.nanosec = static_cast<uint32_t>(
      (current_time.seconds() - static_cast<double>(ros_time.sec)) * 1e9);
    
    // 发布比赛状态
    auto game_status_msg = std::make_unique<rm_referee_ros2::msg::GameStatus>();
    game_status_msg->game_type = 1;  // 假设为RMUC类型
    game_status_msg->game_progress = game_started_ ? 4 : 2;  // 4表示比赛中，2表示准备中
    game_status_msg->stage_remain_time = 180;  // 剩余时间3分钟
    game_status_msg->sync_time_stamp = current_time.seconds() * 1000;  // 时间戳（毫秒）
    game_status_msg->stamp = ros_time;  // ROS 2标准时间戳
    game_status_publisher_->publish(std::move(game_status_msg));

    // 如果有位置数据，发布机器人位置
    if (has_pos_ && !pos_data_stale) {
      auto robot_pos_msg = std::make_unique<rm_referee_ros2::msg::RobotPos>();
      robot_pos_msg->x = current_x_;
      robot_pos_msg->y = current_y_;
      robot_pos_msg->angle = current_angle_;
      robot_pos_msg->stamp = ros_time;  // ROS 2标准时间戳
      robot_pos_publisher_->publish(std::move(robot_pos_msg));
    }

    // 发布机器人状态（每5秒随机减少一点血量和子弹，模拟实际比赛）
    static double last_status_update = 0.0;
    double elapsed = current_time.seconds() - last_status_update;
    
    if (elapsed > 5.0 && game_started_) {
      // 随机减少血量（最多减少10点）
      int hp_decrease = rand() % 11;
      current_hp_ = std::max(0, current_hp_ - hp_decrease);
      
      // 随机减少子弹（最多减少5发）
      int bullet_decrease = rand() % 6;
      current_bullets_ = std::max(0, current_bullets_ - bullet_decrease);
      
      if (hp_decrease > 0 || bullet_decrease > 0) {
        RCLCPP_INFO(this->get_logger(), "状态更新：血量减少%d，子弹减少%d", hp_decrease, bullet_decrease);
      }
      
      last_status_update = current_time.seconds();
    }
    
    // 发布机器人状态
    auto robot_status_msg = std::make_unique<rm_referee_ros2::msg::RobotStatus>();
    robot_status_msg->robot_id = robot_id_;
    robot_status_msg->current_hp = current_hp_;
    robot_status_msg->maximum_hp = initial_hp_;
    robot_status_msg->stamp = ros_time;  // ROS 2标准时间戳
    robot_status_publisher_->publish(std::move(robot_status_msg));

    // 发布子弹剩余
    auto bullet_remaining_msg = std::make_unique<rm_referee_ros2::msg::BulletRemaining>();
    bullet_remaining_msg->bullet_allowance_num_17_mm = current_bullets_;
    bullet_remaining_msg->bullet_allowance_num_42_mm = 0;  // 不使用42mm弹丸
    bullet_remaining_msg->stamp = ros_time;  // ROS 2标准时间戳
    bullet_remaining_publisher_->publish(std::move(bullet_remaining_msg));
  }

  // 发布者
  rclcpp::Publisher<rm_referee_ros2::msg::GameStatus>::SharedPtr game_status_publisher_;
  rclcpp::Publisher<rm_referee_ros2::msg::RobotPos>::SharedPtr robot_pos_publisher_;
  rclcpp::Publisher<rm_referee_ros2::msg::RobotStatus>::SharedPtr robot_status_publisher_;
  rclcpp::Publisher<rm_referee_ros2::msg::BulletRemaining>::SharedPtr bullet_remaining_publisher_;

  // 订阅者
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_subscription_;
  rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr amcl_pose_subscription_;

  // 定时器
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::TimerBase::SharedPtr game_start_timer_;

  // 话题名称
  std::string game_status_topic_;
  std::string robot_pos_topic_;
  std::string robot_status_topic_;
  std::string bullet_remaining_topic_;

  // 状态变量
  bool has_pos_ = false;
  bool game_started_ = false;
  double current_x_ = 0.0;
  double current_y_ = 0.0;
  double current_angle_ = 0.0;
  int current_hp_ = 0;
  int current_bullets_ = 0;
  int initial_hp_ = 500;
  int initial_bullets_ = 300;
  int robot_id_ = 1;
  rclcpp::Time last_pose_time_ = rclcpp::Time(0);
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MockRefereeSystemNode>());
  rclcpp::shutdown();
  return 0;
} 
#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "rclcpp_action/rclcpp_action.hpp"

using namespace std::chrono_literals;
using NavigateToPose = nav2_msgs::action::NavigateToPose;
using GoalHandleNavigateToPose = rclcpp_action::ClientGoalHandle<NavigateToPose>;

/**
 * @brief 固定点导航测试节点，使用nav2进行导航
 */
class FixedPointNavigationNode : public rclcpp::Node
{
public:
  FixedPointNavigationNode()
  : Node("fixed_point_navigation_node"), current_target_(0)
  {
    // 声明参数
    this->declare_parameter("start_delay", 5.0);
    
    // 定义固定目标点坐标（测试用）
    target_points_.push_back({1.0, 0.0, 0.0});  // x, y, yaw
    target_points_.push_back({2.0, 1.0, 1.57});  // x, y, yaw

    // 创建action客户端
    nav_to_pose_client_ = rclcpp_action::create_client<NavigateToPose>(
      this, "navigate_to_pose");

    // 创建定时器，延迟后开始导航
    double start_delay = this->get_parameter("start_delay").as_double();
    timer_ = this->create_wall_timer(
      std::chrono::duration<double>(start_delay), 
      std::bind(&FixedPointNavigationNode::start_navigation, this));

    RCLCPP_INFO(this->get_logger(), "固定点导航节点已初始化，将在%.1f秒后开始导航", start_delay);
    RCLCPP_INFO(this->get_logger(), "总共设置了%zu个导航目标点", target_points_.size());
    for (size_t i = 0; i < target_points_.size(); ++i) {
      RCLCPP_INFO(this->get_logger(), "目标点 %zu: (%.2f, %.2f, %.2f)", 
                 i+1, target_points_[i][0], target_points_[i][1], target_points_[i][2]);
    }
  }

private:
  // 开始导航
  void start_navigation()
  {
    // 注销定时器，不再重复触发
    timer_.reset();

    // 确保action客户端已连接
    if (!nav_to_pose_client_->wait_for_action_server(10s)) {
      RCLCPP_ERROR(this->get_logger(), "导航服务未启动，无法执行导航");
      return;
    }

    // 发送第一个导航目标
    send_goal();
  }

  // 发送导航目标
  void send_goal()
  {
    if (current_target_ >= target_points_.size()) {
      RCLCPP_INFO(this->get_logger(), "所有导航点已完成！");
      return;
    }

    auto goal_msg = NavigateToPose::Goal();
    goal_msg.pose.header.frame_id = "map";
    goal_msg.pose.header.stamp = this->now();
    
    // 设置目标点位置
    goal_msg.pose.pose.position.x = target_points_[current_target_][0];
    goal_msg.pose.pose.position.y = target_points_[current_target_][1];
    goal_msg.pose.pose.position.z = 0.0;
    
    // 设置朝向（四元数）
    double yaw = target_points_[current_target_][2];
    goal_msg.pose.pose.orientation.w = cos(yaw / 2.0);
    goal_msg.pose.pose.orientation.z = sin(yaw / 2.0);
    goal_msg.pose.pose.orientation.x = 0.0;
    goal_msg.pose.pose.orientation.y = 0.0;

    RCLCPP_INFO(this->get_logger(), "发送导航目标 %zu: (%.2f, %.2f, %.2f)", 
               current_target_ + 1, 
               target_points_[current_target_][0], 
               target_points_[current_target_][1], 
               target_points_[current_target_][2]);

    auto send_goal_options = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
    send_goal_options.goal_response_callback =
      std::bind(&FixedPointNavigationNode::goal_response_callback, this, std::placeholders::_1);
    send_goal_options.feedback_callback =
      std::bind(&FixedPointNavigationNode::feedback_callback, this, std::placeholders::_1, std::placeholders::_2);
    send_goal_options.result_callback =
      std::bind(&FixedPointNavigationNode::result_callback, this, std::placeholders::_1);

    nav_to_pose_client_->async_send_goal(goal_msg, send_goal_options);
  }

  // 目标响应回调
  void goal_response_callback(const GoalHandleNavigateToPose::SharedPtr & goal_handle)
  {
    if (!goal_handle) {
      RCLCPP_ERROR(this->get_logger(), "目标被拒绝");
    } else {
      RCLCPP_INFO(this->get_logger(), "目标接受，开始导航");
    }
  }

  // 反馈回调
  void feedback_callback(
    GoalHandleNavigateToPose::SharedPtr,
    const std::shared_ptr<const NavigateToPose::Feedback> feedback)
  {
    RCLCPP_DEBUG(this->get_logger(), "当前位置: %.2f, %.2f", 
               feedback->current_pose.pose.position.x,
               feedback->current_pose.pose.position.y);
  }

  // 结果回调
  void result_callback(const GoalHandleNavigateToPose::WrappedResult & result)
  {
    switch (result.code) {
      case rclcpp_action::ResultCode::SUCCEEDED:
        RCLCPP_INFO(this->get_logger(), "导航成功完成！");
        // 导航到下一个点
        current_target_++;
        send_goal();
        break;
      case rclcpp_action::ResultCode::ABORTED:
        RCLCPP_ERROR(this->get_logger(), "导航被中止");
        break;
      case rclcpp_action::ResultCode::CANCELED:
        RCLCPP_ERROR(this->get_logger(), "导航被取消");
        break;
      default:
        RCLCPP_ERROR(this->get_logger(), "未知的导航结果");
        break;
    }
  }

  // 成员变量
  std::vector<std::array<double, 3>> target_points_;  // x, y, yaw
  size_t current_target_;
  rclcpp_action::Client<NavigateToPose>::SharedPtr nav_to_pose_client_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<FixedPointNavigationNode>());
  rclcpp::shutdown();
  return 0;
} 
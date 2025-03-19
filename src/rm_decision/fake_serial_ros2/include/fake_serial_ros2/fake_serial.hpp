#pragma once

#include <rclcpp/rclcpp.hpp>
#include <rm_referee_ros2/msg/game_status.hpp>
#include <rm_referee_ros2/msg/game_robot_hp.hpp>
#include <rm_referee_ros2/msg/game_result.hpp>
#include <rm_referee_ros2/msg/field_events.hpp>
#include <rm_referee_ros2/msg/robot_hurt.hpp>
#include <rm_referee_ros2/msg/referee_warning.hpp>
#include <rm_referee_ros2/msg/shoot_data.hpp>
#include <rm_referee_ros2/msg/bullet_remaining.hpp>
#include <rm_referee_ros2/msg/robot_status.hpp>
#include <rm_referee_ros2/msg/robot_pos.hpp>

namespace fake_serial_ros2
{

// 如果我们需要使用命令ID，可以自己定义
// 这些枚举常量在fake_serial项目中并不会被实际使用到
enum GameType
{
  GAME_TYPE_NONE = 0,
  GAME_TYPE_OFFICIAL = 1,
  GAME_TYPE_RANKING = 2,
  GAME_TYPE_WARMUP = 3,
  GAME_TYPE_DEMO = 4
};

enum GameProgress
{
  PROGRESS_NONE = 0,
  PROGRESS_PREPARING = 1,
  PROGRESS_SELF_CHECK = 2,
  PROGRESS_COUNTDOWN = 3,
  PROGRESS_PLAYING = 4,
  PROGRESS_END = 5
};

class FakeSerial : public rclcpp::Node
{
public:
  explicit FakeSerial(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~FakeSerial();

  // 发布各种消息的方法
  void publishGameStatus(const rm_referee_ros2::msg::GameStatus & msg);
  void publishGameRobotHp(const rm_referee_ros2::msg::GameRobotHp & msg);
  void publishGameResult(const rm_referee_ros2::msg::GameResult & msg);
  void publishFieldEvents(const rm_referee_ros2::msg::FieldEvents & msg);
  void publishRobotHurt(const rm_referee_ros2::msg::RobotHurt & msg);
  void publishRefereeWarning(const rm_referee_ros2::msg::RefereeWarning & msg);
  void publishShootData(const rm_referee_ros2::msg::ShootData & msg);
  void publishBulletRemaining(const rm_referee_ros2::msg::BulletRemaining & msg);
  void publishRobotStatus(const rm_referee_ros2::msg::RobotStatus & msg);
  void publishRobotPos(const rm_referee_ros2::msg::RobotPos & msg);
  
  // 获取默认消息值
  rm_referee_ros2::msg::GameStatus getDefaultGameStatus();
  rm_referee_ros2::msg::GameRobotHp getDefaultGameRobotHp();
  rm_referee_ros2::msg::GameResult getDefaultGameResult();
  rm_referee_ros2::msg::FieldEvents getDefaultFieldEvents();
  rm_referee_ros2::msg::RobotHurt getDefaultRobotHurt();
  rm_referee_ros2::msg::RefereeWarning getDefaultRefereeWarning();
  rm_referee_ros2::msg::ShootData getDefaultShootData();
  rm_referee_ros2::msg::BulletRemaining getDefaultBulletRemaining();
  rm_referee_ros2::msg::RobotStatus getDefaultRobotStatus();
  rm_referee_ros2::msg::RobotPos getDefaultRobotPos();

private:
  // 定时器
  rclcpp::TimerBase::SharedPtr timer_;
  
  // 各种消息的发布者
  rclcpp::Publisher<rm_referee_ros2::msg::GameStatus>::SharedPtr game_status_pub_;
  rclcpp::Publisher<rm_referee_ros2::msg::GameRobotHp>::SharedPtr game_robot_hp_pub_;
  rclcpp::Publisher<rm_referee_ros2::msg::GameResult>::SharedPtr game_result_pub_;
  rclcpp::Publisher<rm_referee_ros2::msg::FieldEvents>::SharedPtr field_events_pub_;
  rclcpp::Publisher<rm_referee_ros2::msg::RobotHurt>::SharedPtr robot_hurt_pub_;
  rclcpp::Publisher<rm_referee_ros2::msg::RefereeWarning>::SharedPtr referee_warning_pub_;
  rclcpp::Publisher<rm_referee_ros2::msg::ShootData>::SharedPtr shoot_data_pub_;
  rclcpp::Publisher<rm_referee_ros2::msg::BulletRemaining>::SharedPtr bullet_remaining_pub_;
  rclcpp::Publisher<rm_referee_ros2::msg::RobotStatus>::SharedPtr robot_status_pub_;
  rclcpp::Publisher<rm_referee_ros2::msg::RobotPos>::SharedPtr robot_pos_pub_;
  
  // 当前消息值
  rm_referee_ros2::msg::GameStatus current_game_status_;
  rm_referee_ros2::msg::GameRobotHp current_game_robot_hp_;
  rm_referee_ros2::msg::GameResult current_game_result_;
  rm_referee_ros2::msg::FieldEvents current_field_events_;
  rm_referee_ros2::msg::RobotHurt current_robot_hurt_;
  rm_referee_ros2::msg::RefereeWarning current_referee_warning_;
  rm_referee_ros2::msg::ShootData current_shoot_data_;
  rm_referee_ros2::msg::BulletRemaining current_bullet_remaining_;
  rm_referee_ros2::msg::RobotStatus current_robot_status_;
  rm_referee_ros2::msg::RobotPos current_robot_pos_;
};

} // namespace fake_serial_ros2 
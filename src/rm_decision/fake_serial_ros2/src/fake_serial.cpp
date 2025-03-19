#include "fake_serial_ros2/fake_serial.hpp"
#include <chrono>

namespace fake_serial_ros2
{

FakeSerial::FakeSerial(const rclcpp::NodeOptions & options)
: Node("fake_serial", options)
{
  // 创建发布者
  game_status_pub_ = this->create_publisher<rm_referee_ros2::msg::GameStatus>("/referee/game_status", 10);
  game_robot_hp_pub_ = this->create_publisher<rm_referee_ros2::msg::GameRobotHp>("/referee/game_robot_hp", 10);
  game_result_pub_ = this->create_publisher<rm_referee_ros2::msg::GameResult>("/referee/game_result", 5);
  field_events_pub_ = this->create_publisher<rm_referee_ros2::msg::FieldEvents>("/referee/field_events", 10);
  robot_hurt_pub_ = this->create_publisher<rm_referee_ros2::msg::RobotHurt>("/referee/robot_hurt", 10);
  referee_warning_pub_ = this->create_publisher<rm_referee_ros2::msg::RefereeWarning>("/referee/referee_warning", 5);
  shoot_data_pub_ = this->create_publisher<rm_referee_ros2::msg::ShootData>("/referee/shoot_data", 10);
  bullet_remaining_pub_ = this->create_publisher<rm_referee_ros2::msg::BulletRemaining>("/referee/bullet_remaining", 10);
  robot_status_pub_ = this->create_publisher<rm_referee_ros2::msg::RobotStatus>("/referee/robot_status", 10);
  robot_pos_pub_ = this->create_publisher<rm_referee_ros2::msg::RobotPos>("/referee/robot_pos", 10);
  
  // 初始化当前消息为默认值
  current_game_status_ = getDefaultGameStatus();
  current_game_robot_hp_ = getDefaultGameRobotHp();
  current_game_result_ = getDefaultGameResult();
  current_field_events_ = getDefaultFieldEvents();
  current_robot_hurt_ = getDefaultRobotHurt();
  current_referee_warning_ = getDefaultRefereeWarning();
  current_shoot_data_ = getDefaultShootData();
  current_bullet_remaining_ = getDefaultBulletRemaining();
  current_robot_status_ = getDefaultRobotStatus();
  current_robot_pos_ = getDefaultRobotPos();
}

FakeSerial::~FakeSerial()
{
  // 析构函数不需要特殊处理
}

// 发布消息的实现
void FakeSerial::publishGameStatus(const rm_referee_ros2::msg::GameStatus & msg)
{
  current_game_status_ = msg;
  game_status_pub_->publish(msg);
  RCLCPP_INFO(this->get_logger(), "已发布比赛状态数据");
}

void FakeSerial::publishGameRobotHp(const rm_referee_ros2::msg::GameRobotHp & msg)
{
  current_game_robot_hp_ = msg;
  game_robot_hp_pub_->publish(msg);
  RCLCPP_INFO(this->get_logger(), "已发布机器人血量数据");
}

void FakeSerial::publishGameResult(const rm_referee_ros2::msg::GameResult & msg)
{
  current_game_result_ = msg;
  game_result_pub_->publish(msg);
  RCLCPP_INFO(this->get_logger(), "已发布比赛结果数据");
}

void FakeSerial::publishFieldEvents(const rm_referee_ros2::msg::FieldEvents & msg)
{
  current_field_events_ = msg;
  field_events_pub_->publish(msg);
  RCLCPP_INFO(this->get_logger(), "已发布场地事件数据");
}

void FakeSerial::publishRobotHurt(const rm_referee_ros2::msg::RobotHurt & msg)
{
  current_robot_hurt_ = msg;
  robot_hurt_pub_->publish(msg);
  RCLCPP_INFO(this->get_logger(), "已发布机器人伤害数据");
}

void FakeSerial::publishRefereeWarning(const rm_referee_ros2::msg::RefereeWarning & msg)
{
  current_referee_warning_ = msg;
  referee_warning_pub_->publish(msg);
  RCLCPP_INFO(this->get_logger(), "已发布裁判警告数据");
}

void FakeSerial::publishShootData(const rm_referee_ros2::msg::ShootData & msg)
{
  current_shoot_data_ = msg;
  shoot_data_pub_->publish(msg);
  RCLCPP_INFO(this->get_logger(), "已发布射击数据");
}

void FakeSerial::publishBulletRemaining(const rm_referee_ros2::msg::BulletRemaining & msg)
{
  current_bullet_remaining_ = msg;
  bullet_remaining_pub_->publish(msg);
  RCLCPP_INFO(this->get_logger(), "已发布子弹剩余数据");
}

void FakeSerial::publishRobotStatus(const rm_referee_ros2::msg::RobotStatus & msg)
{
  current_robot_status_ = msg;
  robot_status_pub_->publish(msg);
  RCLCPP_INFO(this->get_logger(), "已发布机器人状态数据");
}

void FakeSerial::publishRobotPos(const rm_referee_ros2::msg::RobotPos & msg)
{
  current_robot_pos_ = msg;
  robot_pos_pub_->publish(msg);
  RCLCPP_INFO(this->get_logger(), "已发布机器人位置数据");
}

// 默认消息值的实现
rm_referee_ros2::msg::GameStatus FakeSerial::getDefaultGameStatus()
{
  rm_referee_ros2::msg::GameStatus msg;
  msg.game_type = 1;  // 正式比赛
  msg.game_progress = 4;  // 比赛中
  msg.stage_remain_time = 120;  // 剩余120秒
  return msg;
}

rm_referee_ros2::msg::GameRobotHp FakeSerial::getDefaultGameRobotHp()
{
  rm_referee_ros2::msg::GameRobotHp msg;
  
  // 设置默认血量值（满血）
  msg.red_1_robot_hp = 300;  // 英雄
  msg.red_2_robot_hp = 500;  // 工程
  msg.red_3_robot_hp = 200;  // 步兵3
  msg.red_4_robot_hp = 200;  // 步兵4
  msg.red_5_robot_hp = 200;  // 步兵5
  msg.red_7_robot_hp = 600;  // 哨兵
  msg.red_outpost_hp = 1500; // 前哨站
  msg.red_base_hp = 5000;    // 基地
  
  msg.blue_1_robot_hp = 300;  // 英雄
  msg.blue_2_robot_hp = 500;  // 工程
  msg.blue_3_robot_hp = 200;  // 步兵3
  msg.blue_4_robot_hp = 200;  // 步兵4
  msg.blue_5_robot_hp = 200;  // 步兵5
  msg.blue_7_robot_hp = 600;  // 哨兵
  msg.blue_outpost_hp = 1500; // 前哨站
  msg.blue_base_hp = 5000;    // 基地
  
  return msg;
}

rm_referee_ros2::msg::GameResult FakeSerial::getDefaultGameResult()
{
  rm_referee_ros2::msg::GameResult msg;
  msg.winner = 0;  // 平局
  return msg;
}

rm_referee_ros2::msg::FieldEvents FakeSerial::getDefaultFieldEvents()
{
  rm_referee_ros2::msg::FieldEvents msg;
  msg.event_type = 0;  // 无事件
  return msg;
}

rm_referee_ros2::msg::RobotHurt FakeSerial::getDefaultRobotHurt()
{
  rm_referee_ros2::msg::RobotHurt msg;
  msg.armor_id = 0;  // 前装甲板
  msg.hurt_type = 0;  // 装甲伤害
  return msg;
}

rm_referee_ros2::msg::RefereeWarning FakeSerial::getDefaultRefereeWarning()
{
  rm_referee_ros2::msg::RefereeWarning msg;
  msg.level = 0;  // 无警告
  msg.foul_robot_id = 0;
  msg.count = 0;
  return msg;
}

rm_referee_ros2::msg::ShootData FakeSerial::getDefaultShootData()
{
  rm_referee_ros2::msg::ShootData msg;
  msg.bullet_type = 1;  // 17mm弹丸
  msg.shooter_id = 1;   // 发射机构1
  msg.bullet_freq = 10; // 10Hz射频
  msg.bullet_speed = 15.0f;  // 15m/s射速
  return msg;
}

rm_referee_ros2::msg::BulletRemaining FakeSerial::getDefaultBulletRemaining()
{
  rm_referee_ros2::msg::BulletRemaining msg;
  msg.bullet_allowance_num_17_mm = 100;  // 17mm子弹100发
  msg.bullet_allowance_num_42_mm = 10;   // 42mm子弹10发
  msg.coin_remaining_num = 50;           // 50金币
  return msg;
}

rm_referee_ros2::msg::RobotStatus FakeSerial::getDefaultRobotStatus()
{
  rm_referee_ros2::msg::RobotStatus msg;
  msg.robot_id = 3;  // 红方步兵3
  msg.robot_level = 1;  // 1级
  msg.current_hp = 200;  // 当前血量
  msg.maximum_hp = 200;  // 最大血量
  msg.shooter_barrel_cooling_value = 0;  // 枪管冷却值
  msg.shooter_barrel_heat_limit = 240;   // 枪管热量上限
  msg.chassis_power_limit = 60;          // 底盘功率限制
  msg.power_management_gimbal_output = 1;  // 云台输出允许
  msg.power_management_chassis_output = 1; // 底盘输出允许
  msg.power_management_shooter_output = 1; // 射击输出允许
  return msg;
}

rm_referee_ros2::msg::RobotPos FakeSerial::getDefaultRobotPos()
{
  rm_referee_ros2::msg::RobotPos msg;
  msg.x = 0.0;  // X坐标
  msg.y = 0.0;  // Y坐标
  msg.angle = 0.0;  // 角度
  return msg;
}

} // namespace fake_serial_ros2 
/**
  ****************************(C) COPYRIGHT 2023 Polarbear*************************
  * @file       rm_serial_driver.cpp
  * @brief      串口通信模块
  * @note       感谢@ChenJun创建本模块并开源，
  *             现内容为北极熊基于开源模块进行修改并适配自己的车车后的结果。
  * @history
  *  Version    Date            Author          Modification
  *  V1.0.0     2022            ChenJun         1. done
  *  V1.0.1     2023-12-11      Penguin         1. 添加与rm_rune_dector_node模块连接的Client
  *  V1.0.2     2024-3-1        LihanChen       1. 添加导航数据包，并重命名packet和相关函数
  *
  @verbatim
  =================================================================================

  =================================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2023 Polarbear*************************
  */

#include <tf2/LinearMath/Quaternion.h>

#include <rclcpp/logging.hpp>
#include <rclcpp/qos.hpp>
#include <rclcpp/utilities.hpp>
#include <rm_referee_ros2/msg/robot_status.hpp>
#include <rm_referee_ros2/msg/shoot_data.hpp>
#include <serial_driver/serial_driver.hpp>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

// C++ system
#include <cstdint>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "rm_serial_driver/crc.hpp"
#include "rm_serial_driver/packet.hpp"
#include "rm_serial_driver/rm_serial_driver.hpp"

namespace rm_serial_driver
{
RMSerialDriver::RMSerialDriver(const rclcpp::NodeOptions & options)
: Node("rm_serial_driver", options),
  owned_ctx_{new IoContext(2)},
  serial_driver_{new drivers::serial_driver::SerialDriver(*owned_ctx_)}
{
  RCLCPP_INFO(get_logger(), "Start RMSerialDriver!");

  getParams();

  // TF broadcaster
  timestamp_offset_ = this->declare_parameter("timestamp_offset", 0.0);
  tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

  // Create Publisher
  latency_pub_ = this->create_publisher<std_msgs::msg::Float64>("/latency", 10);
  marker_pub_ = this->create_publisher<visualization_msgs::msg::Marker>("/aiming_point", 10);

  // Detect parameter client
  detector_param_client_ = std::make_shared<rclcpp::AsyncParametersClient>(this, "armor_detector");
  rune_detector_param_client_ =
    std::make_shared<rclcpp::AsyncParametersClient>(this, "rm_rune_detector");

  // Tracker reset service client
  reset_tracker_client_ = this->create_client<std_srvs::srv::Trigger>("/tracker/reset");

  try {
    serial_driver_->init_port(device_name_, *device_config_);
    if (!serial_driver_->port()->is_open()) {
      serial_driver_->port()->open();
      receive_thread_ = std::thread(&RMSerialDriver::receiveDataVision, this);
    }
  } catch (const std::exception & ex) {
    RCLCPP_ERROR(
      get_logger(), "Error creating serial port: %s - %s", device_name_.c_str(), ex.what());
    throw ex;
  }

  aiming_point_.header.frame_id = "odom";
  aiming_point_.ns = "aiming_point";
  aiming_point_.type = visualization_msgs::msg::Marker::SPHERE;
  aiming_point_.action = visualization_msgs::msg::Marker::ADD;
  aiming_point_.scale.x = aiming_point_.scale.y = aiming_point_.scale.z = 0.12;
  aiming_point_.color.r = 1.0;
  aiming_point_.color.g = 1.0;
  aiming_point_.color.b = 1.0;
  aiming_point_.color.a = 1.0;
  aiming_point_.lifetime = rclcpp::Duration::from_seconds(0.1);

  // Create Subscription
  target_sub_ = this->create_subscription<auto_aim_interfaces::msg::Target>(
    "/tracker/target", rclcpp::SensorDataQoS(),
    std::bind(&RMSerialDriver::sendDataVision, this, std::placeholders::_1));
  cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
    "/cmd_vel_chassis", 10, std::bind(&RMSerialDriver::sendDataTwist, this, std::placeholders::_1));
  // Create Subscription for shooting data
  shoot_data_sub_ = this->create_subscription<rm_referee_ros2::msg::ShootData>(
    "/referee/shoot_data", 10,
    std::bind(&RMSerialDriver::sendShootData, this, std::placeholders::_1));
  robot_status_sub_ = this->create_subscription<rm_referee_ros2::msg::RobotStatus>(
    "/referee/robot_status", 10,
    std::bind(&RMSerialDriver::sendRobotStatus, this, std::placeholders::_1));
}

RMSerialDriver::~RMSerialDriver()
{
  if (receive_thread_.joinable()) {
    receive_thread_.join();
  }

  if (serial_driver_->port()->is_open()) {
    serial_driver_->port()->close();
  }

  if (owned_ctx_) {
    owned_ctx_->waitForExit();
  }
}

// void RMSerialDriver::receiveDataVision()
// {
//   std::vector<uint8_t> header(1);
//   std::vector<uint8_t> data;
//   data.reserve(sizeof(ReceivePacketVision));

//   while (rclcpp::ok()) {
//     try {
//       serial_driver_->port()->receive(header);

//       if (header[0] == 0x5A) {
//         data.resize(sizeof(ReceivePacketVision) - 1);
//         serial_driver_->port()->receive(data);

//         data.insert(data.begin(), header[0]);
//         ReceivePacketVision packet = fromVector<ReceivePacketVision>(data);

//         bool crc_ok =
//           crc16::Verify_CRC16_Check_Sum(reinterpret_cast<const uint8_t *>(&packet), sizeof(packet));
//         if (crc_ok) {
//           uint8_t detect_color = packet.flags & 0x01;
//           if (!initial_set_param_ || detect_color != previous_receive_color_) {
//             setParam(rclcpp::Parameter("detect_color", detect_color));
//             previous_receive_color_ = detect_color;
//           }

//           if (packet.flags & 0x02) {
//             resetTracker();
//           }

//           geometry_msgs::msg::TransformStamped t;
//           timestamp_offset_ = this->get_parameter("timestamp_offset").as_double();
//           t.header.stamp = this->now() + rclcpp::Duration::from_seconds(timestamp_offset_);
//           t.header.frame_id = "odom";
//           t.child_frame_id = "gimbal_link";
//           tf2::Quaternion q;
//           q.setRPY(packet.roll, packet.pitch, packet.yaw);
//           t.transform.rotation = tf2::toMsg(q);
//           tf_broadcaster_->sendTransform(t);

//           if (abs(packet.aim_x) > 0.01) {
//             aiming_point_.header.stamp = this->now();
//             aiming_point_.pose.position.x = packet.aim_x;
//             aiming_point_.pose.position.y = packet.aim_y;
//             aiming_point_.pose.position.z = packet.aim_z;
//             marker_pub_->publish(aiming_point_);
//           }
//         } else {
//           RCLCPP_ERROR(get_logger(), "CRC error!");
//         }
//       } else {
//         RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 20, "Invalid header: %02X", header[0]);
//       }
//     } catch (const std::exception & ex) {
//       RCLCPP_ERROR_THROTTLE(
//         get_logger(), *get_clock(), 20, "Error while receiving data: %s", ex.what());
//       reopenPort();
//     }
//   }
// }

void RMSerialDriver::receiveDataVision()
{
  // 1. 预分配内存（移出循环，避免频繁 new/delete）
  std::vector<uint8_t> header(1);
  std::vector<uint8_t> data;
  // 预留足够空间，避免 vector 自动扩容带来的拷贝开销
  data.reserve(sizeof(ReceivePacketVision));

  while (rclcpp::ok()) {
    try {
      // --- 阶段一：同步（寻找帧头）---
      // 持续读取单字节，直到找到帧头 0x5A
      while (rclcpp::ok()) {
        serial_driver_->port()->receive(header);

        if (header.empty()) {
          continue;  // 超时未读到数据，重试
        }

        if (header[0] == 0x5A) {
          break;  // 找到帧头
        }
        // 如果不是帧头，继续循环读取下一个字节，直到找到为止
        // 可选：在这里添加一些调试打印，但不要太频繁
      }

      // --- 阶段二：读取负载 ---
      // 既然已经拿到了 0x5A，那么剩下的数据长度是确定的
      // 大小 = 结构体总大小 - 1 (帧头已被读取)
      data.resize(sizeof(ReceivePacketVision) - 1);
      serial_driver_->port()->receive(data);

      // --- 阶段三：重组与校验 ---
      // 将帧头插回数据最前端，还原完整的包以便校验
      data.insert(data.begin(), header[0]);

      // 使用模板函数转换数据
      ReceivePacketVision packet = fromVector<ReceivePacketVision>(data);

      bool crc_ok =
        crc16::Verify_CRC16_Check_Sum(reinterpret_cast<const uint8_t *>(&packet), sizeof(packet));

      if (crc_ok) {
        // --- 修改开始：打印接收成功日志 ---
        // 使用限速打印（每 1000 毫秒打印一次），避免高频数据刷屏
        RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 1000, "Receive packet successfully!");

        // --- 阶段四：业务逻辑（保持原样）---
        uint8_t detect_color = packet.flags & 0x01;
        if (!initial_set_param_ || detect_color != previous_receive_color_) {
          setParam(rclcpp::Parameter("detect_color", detect_color));
          previous_receive_color_ = detect_color;
        }

        if (packet.flags & 0x02) {
          resetTracker();
        }

        geometry_msgs::msg::TransformStamped t;
        timestamp_offset_ = this->get_parameter("timestamp_offset").as_double();
        t.header.stamp = this->now() + rclcpp::Duration::from_seconds(timestamp_offset_);
        t.header.frame_id = "odom";
        t.child_frame_id = "gimbal_link";
        tf2::Quaternion q;
        q.setRPY(packet.roll, packet.pitch, packet.yaw);
        t.transform.rotation = tf2::toMsg(q);
        tf_broadcaster_->sendTransform(t);

        if (abs(packet.aim_x) > 0.01) {
          aiming_point_.header.stamp = this->now();
          aiming_point_.pose.position.x = packet.aim_x;
          aiming_point_.pose.position.y = packet.aim_y;
          aiming_point_.pose.position.z = packet.aim_z;
          marker_pub_->publish(aiming_point_);
        }
      } else {
        // CRC 校验失败
        // 注意：这里不需要做特殊处理，循环会回到开头，重新寻找下一个 0x5A
        // RCLCPP_ERROR(get_logger(), "CRC error!");
        // --- 修改开始：增强 CRC 错误日志 ---
        RCLCPP_ERROR(get_logger(), "CRC error!");

        // 1. 获取指向结构体数据的指针
        const uint8_t * raw_data = reinterpret_cast<const uint8_t *>(&packet);
        size_t data_len = sizeof(packet);

        // 2. 手动计算期望的 CRC 值
        // 注意：根据 crc.cpp，CRC16_INIT 为 0xFFFF。
        // 计算范围是：总长度 - 2字节（最后的CRC位）
        uint16_t expected_crc = crc16::Get_CRC16_Check_Sum(raw_data, data_len - 2, 0xFFFF);

        // 3. 解析实际接收到的 CRC 值
        // 根据 crc.cpp 的 Append 函数：倒数第2个字节是低位，倒数第1个字节是高位
        uint16_t actual_crc = (static_cast<uint16_t>(raw_data[data_len - 1]) << 8) |
                              static_cast<uint16_t>(raw_data[data_len - 2]);

        // 打印 CRC 对比信息
        RCLCPP_ERROR(
          get_logger(), "Expected CRC: 0x%04X, Actual CRC: 0x%04X", expected_crc, actual_crc);

        // 4. 将结构体内容转换为 HEX 字符串并打印
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0');
        for (size_t i = 0; i < data_len; ++i) {
          ss << std::setw(2) << static_cast<int>(raw_data[i]) << " ";
        }
        RCLCPP_ERROR(get_logger(), "Received Data (HEX): %s", ss.str().c_str());
      }

    } catch (const std::exception & ex) {
      // 串口底层错误（如拔掉USB）
      RCLCPP_ERROR_THROTTLE(
        get_logger(), *get_clock(), 20, "Error while receiving data: %s", ex.what());
      reopenPort();
    }
  }
}

void RMSerialDriver::sendDataVision(const auto_aim_interfaces::msg::Target::SharedPtr msg)
{
  const static std::map<std::string, uint8_t> ID_UNIT8_MAP{
    {"", 0},  {"outpost", 0}, {"1", 1}, {"1", 1},     {"2", 2},
    {"3", 3}, {"4", 4},       {"5", 5}, {"guard", 6}, {"base", 7}};

  try {
    SendPacketVision packet;
    packet.tracking = msg->tracking;
    packet.id = ID_UNIT8_MAP.at(msg->id);
    packet.armors_num = msg->armors_num;
    packet.x = msg->position.x;
    packet.y = msg->position.y;
    packet.z = msg->position.z;
    packet.yaw = msg->yaw;
    packet.vx = msg->velocity.x;
    packet.vy = msg->velocity.y;
    packet.vz = msg->velocity.z;
    packet.v_yaw = msg->v_yaw;
    packet.r1 = msg->radius_1;
    packet.r2 = msg->radius_2;
    packet.dz = msg->dz;
    crc16::Append_CRC16_Check_Sum(reinterpret_cast<uint8_t *>(&packet), sizeof(packet));

    std::vector<uint8_t> data = toVector(packet);

    serial_driver_->port()->send(data);

    std_msgs::msg::Float64 latency;
    latency.data = (this->now() - msg->header.stamp).seconds() * 1000.0;
    RCLCPP_DEBUG_STREAM(get_logger(), "Total latency: " + std::to_string(latency.data) + "ms");
    latency_pub_->publish(latency);
  } catch (const std::exception & ex) {
    RCLCPP_ERROR(get_logger(), "Error while sending data: %s", ex.what());
    reopenPort();
  }
}

void RMSerialDriver::sendDataTwist(const geometry_msgs::msg::Twist::SharedPtr msg)
{
  try {
    SendPacketTwist packet;
    packet.linear_x = msg->linear.x;
    packet.linear_y = msg->linear.y;
    packet.linear_z = msg->linear.z;
    packet.angular_x = msg->angular.x;
    packet.angular_y = msg->angular.y;
    packet.angular_z = msg->angular.z;
    crc16::Append_CRC16_Check_Sum(reinterpret_cast<uint8_t *>(&packet), sizeof(packet));

    std::vector<uint8_t> data = toVector(packet);

    serial_driver_->port()->send(data);

  } catch (const std::exception & ex) {
    RCLCPP_ERROR(get_logger(), "Error while sending data: %s", ex.what());
    reopenPort();
  }
}

// 发送射击数据的函数
void RMSerialDriver::sendShootData(const rm_referee_ros2::msg::ShootData::SharedPtr msg)
{
  try {
    ShootDataPacket packet;  // 创建数据包结构体

    // 复制数据
    packet.bullet_type = msg->bullet_type;
    packet.shooter_id = msg->shooter_id;
    packet.bullet_freq = msg->bullet_freq;
    packet.bullet_speed = msg->bullet_speed;

    // 计算 CRC 校验和
    crc16::Append_CRC16_Check_Sum(reinterpret_cast<uint8_t *>(&packet), sizeof(packet));

    std::vector<uint8_t> data = toVector(packet);

    serial_driver_->port()->send(data);

  } catch (const std::exception & ex) {
    RCLCPP_ERROR(get_logger(), "Error while sending shoot data: %s", ex.what());
    reopenPort();  // 异常处理和重启串口
  }
}

// 发送比赛机器人血量数据的函数
void RMSerialDriver::sendRobotStatus(const rm_referee_ros2::msg::RobotStatus::SharedPtr msg)
{
  try {
    RobotStatusPacket packet;  // 创建数据包结构体

    // 复制数据
    packet.robot_id = msg->robot_id;
    packet.robot_level = msg->robot_level;
    packet.current_hp = msg->current_hp;
    packet.maximum_hp = msg->maximum_hp;
    packet.shooter_barrel_cooling_value = msg->shooter_barrel_cooling_value;
    packet.shooter_barrel_heat_limit = msg->shooter_barrel_heat_limit;
    packet.chassis_power_limit = msg->chassis_power_limit;
    // packet.power_management_gimbal_output = msg->power_management_gimbal_output;
    // packet.power_management_chassis_output = msg->power_management_chassis_output;
    // packet.power_management_shooter_output = msg->power_management_shooter_output;

    // 修改处：手动进行打包 (Packing)
    packet.power_management = 0x00;  // 初始化
    if (msg->power_management_gimbal_output) packet.power_management |= (1 << 0);
    if (msg->power_management_chassis_output) packet.power_management |= (1 << 1);
    if (msg->power_management_shooter_output) packet.power_management |= (1 << 2);

    // 计算 CRC 校验和
    crc16::Append_CRC16_Check_Sum(reinterpret_cast<uint8_t *>(&packet), sizeof(packet));

    std::vector<uint8_t> data = toVector(packet);

    serial_driver_->port()->send(data);

  } catch (const std::exception & ex) {
    RCLCPP_ERROR(get_logger(), "Error while sending robot status: %s", ex.what());
    reopenPort();  // 异常处理和重启串口
  }
}

void RMSerialDriver::getParams()
{
  using FlowControl = drivers::serial_driver::FlowControl;
  using Parity = drivers::serial_driver::Parity;
  using StopBits = drivers::serial_driver::StopBits;

  uint32_t baud_rate{};
  auto fc = FlowControl::NONE;
  auto pt = Parity::NONE;
  auto sb = StopBits::ONE;

  try {
    device_name_ = declare_parameter<std::string>("device_name", "");
  } catch (rclcpp::ParameterTypeException & ex) {
    RCLCPP_ERROR(get_logger(), "The device name provided was invalid");
    throw ex;
  }

  try {
    baud_rate = declare_parameter<int>("baud_rate", 0);
  } catch (rclcpp::ParameterTypeException & ex) {
    RCLCPP_ERROR(get_logger(), "The baud_rate provided was invalid");
    throw ex;
  }

  try {
    const auto fc_string = declare_parameter<std::string>("flow_control", "");

    if (fc_string == "none") {
      fc = FlowControl::NONE;
    } else if (fc_string == "hardware") {
      fc = FlowControl::HARDWARE;
    } else if (fc_string == "software") {
      fc = FlowControl::SOFTWARE;
    } else {
      throw std::invalid_argument{
        "The flow_control parameter must be one of: none, software, or hardware."};
    }
  } catch (rclcpp::ParameterTypeException & ex) {
    RCLCPP_ERROR(get_logger(), "The flow_control provided was invalid");
    throw ex;
  }

  try {
    const auto pt_string = declare_parameter<std::string>("parity", "");

    if (pt_string == "none") {
      pt = Parity::NONE;
    } else if (pt_string == "odd") {
      pt = Parity::ODD;
    } else if (pt_string == "even") {
      pt = Parity::EVEN;
    } else {
      throw std::invalid_argument{"The parity parameter must be one of: none, odd, or even."};
    }
  } catch (rclcpp::ParameterTypeException & ex) {
    RCLCPP_ERROR(get_logger(), "The parity provided was invalid");
    throw ex;
  }

  try {
    const auto sb_string = declare_parameter<std::string>("stop_bits", "");

    if (sb_string == "1" || sb_string == "1.0") {
      sb = StopBits::ONE;
    } else if (sb_string == "1.5") {
      sb = StopBits::ONE_POINT_FIVE;
    } else if (sb_string == "2" || sb_string == "2.0") {
      sb = StopBits::TWO;
    } else {
      throw std::invalid_argument{"The stop_bits parameter must be one of: 1, 1.5, or 2."};
    }
  } catch (rclcpp::ParameterTypeException & ex) {
    RCLCPP_ERROR(get_logger(), "The stop_bits provided was invalid");
    throw ex;
  }

  device_config_ =
    std::make_unique<drivers::serial_driver::SerialPortConfig>(baud_rate, fc, pt, sb);
}

void RMSerialDriver::reopenPort()
{
  RCLCPP_WARN(get_logger(), "Attempting to reopen port");
  try {
    if (serial_driver_->port()->is_open()) {
      serial_driver_->port()->close();
    }
    serial_driver_->port()->open();
    RCLCPP_INFO(get_logger(), "Successfully reopened port");
  } catch (const std::exception & ex) {
    RCLCPP_ERROR(get_logger(), "Error while reopening port: %s", ex.what());
    if (rclcpp::ok()) {
      rclcpp::sleep_for(std::chrono::seconds(1));
      reopenPort();
    }
  }
}

void RMSerialDriver::setParam(const rclcpp::Parameter & param)
{
  if (!detector_param_client_->service_is_ready()) {
    RCLCPP_WARN(get_logger(), "Armor service not ready, skipping parameter set");
    return;
  }

  if (
    !set_param_future_.valid() ||
    set_param_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    RCLCPP_INFO(get_logger(), "Setting armor detect_color to %ld...", param.as_int());

    set_param_future_ = detector_param_client_->set_parameters(
      {param}, [this, param](const ResultFuturePtr & results) {
        for (const auto & result : results.get()) {
          if (!result.successful) {
            RCLCPP_ERROR(get_logger(), "Failed to set parameter: %s", result.reason.c_str());
            return;
          }
        }
        RCLCPP_INFO(get_logger(), "Successfully set armor detect_color to %ld!", param.as_int());
        initial_set_param_ = true;
      });
  }

  if (!rune_detector_param_client_->service_is_ready()) {
    RCLCPP_WARN(get_logger(), "Rune service not ready, skipping parameter set");
    return;
  }

  if (
    !set_rune_detector_param_future_.valid() ||
    set_rune_detector_param_future_.wait_for(std::chrono::seconds(0)) ==
      std::future_status::ready) {
    RCLCPP_INFO(get_logger(), "Setting rune detect_color to %ld...", param.as_int());
    set_rune_detector_param_future_ = rune_detector_param_client_->set_parameters(
      {param}, [this, param](const ResultFuturePtr & results) {
        for (const auto & result : results.get()) {
          if (!result.successful) {
            RCLCPP_ERROR(get_logger(), "Failed to set parameter: %s", result.reason.c_str());
            return;
          }
        }
        RCLCPP_INFO(get_logger(), "Successfully set rune detect_color to %ld!", 1 - param.as_int());
      });
  }
}

void RMSerialDriver::resetTracker()
{
  if (!reset_tracker_client_->service_is_ready()) {
    RCLCPP_WARN(get_logger(), "Service not ready, skipping tracker reset");
    return;
  }

  auto request = std::make_shared<std_srvs::srv::Trigger::Request>();
  reset_tracker_client_->async_send_request(request);
  RCLCPP_INFO(get_logger(), "Reset tracker!");
}

}  // namespace rm_serial_driver

#include "rclcpp_components/register_node_macro.hpp"

// Register the component with class_loader.
// This acts as a sort of entry point, allowing the component to be discoverable when its library
// is being loaded into a running process.
RCLCPP_COMPONENTS_REGISTER_NODE(rm_serial_driver::RMSerialDriver)

/**
  ****************************(C) COPYRIGHT 2026 森林狼*************************
  * @file       rm_serial_driver.cpp
  * @brief      RoboMaster 串口通信驱动模块
  * @note       基于 ChenJun 的开源模块进行适配修改。
  *             本模块主要负责视觉数据接收、云台/底盘控制指令发送以及裁判系统数据转发。
  * @history
  *  Version    Date            Author          Modification
  *  V1.0.0     2022            ChenJun         1. done
  *  V1.0.1     2023-12-11      Penguin         1. 添加与rm_rune_dector_node模块连接的Client
  *  V1.0.2     2024-3-1        LihanChen       1. 添加导航数据包，并重命名packet和相关函数
  *  V1.0.3     2026-1-29       VaporTang       1. 修复原有接收数据帧头读取问题
  @verbatim
  =================================================================================

  =================================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2026 森林狼*************************
  */

// ROS 2 Headers
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

namespace rm_serial_driver {
RMSerialDriver::RMSerialDriver(const rclcpp::NodeOptions& options)
    : Node("rm_serial_driver", options),
      owned_ctx_{new IoContext(2)},
      serial_driver_{new drivers::serial_driver::SerialDriver(*owned_ctx_)} {
  RCLCPP_INFO(get_logger(), "Start RMSerialDriver!");

  // Initialize parameters
  getParams();

  // TF broadcaster
  timestamp_offset_ = this->declare_parameter("timestamp_offset", 0.0);
  tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

  // Publishers
  latency_pub_ = this->create_publisher<std_msgs::msg::Float64>("/latency", 10);
  marker_pub_ = this->create_publisher<visualization_msgs::msg::Marker>("/aiming_point", 10);

  // Detect parameter client
  detector_param_client_ = std::make_shared<rclcpp::AsyncParametersClient>(this, "armor_detector");
  rune_detector_param_client_ =
      std::make_shared<rclcpp::AsyncParametersClient>(this, "rm_rune_detector");

  // Tracker reset service client
  reset_tracker_client_ = this->create_client<std_srvs::srv::Trigger>("/tracker/reset");

  // Initialize serial port
  try {
    serial_driver_->init_port(device_name_, *device_config_);
    if (!serial_driver_->port()->is_open()) {
      serial_driver_->port()->open();
      receive_thread_ = std::thread(&RMSerialDriver::receiveDataVision, this);
    }
  } catch (const std::exception& ex) {
    RCLCPP_ERROR(
        get_logger(), "Error creating serial port: %s - %s", device_name_.c_str(), ex.what()
    );
    throw ex;
  }

  // Initialize visualization Marker properties
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
      "/tracker/target",
      rclcpp::SensorDataQoS(),
      std::bind(&RMSerialDriver::sendDataVision, this, std::placeholders::_1)
  );
  cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel_chassis", 10, std::bind(&RMSerialDriver::sendDataTwist, this, std::placeholders::_1)
  );
  // Create Subscription for shooting data
  shoot_data_sub_ = this->create_subscription<rm_referee_ros2::msg::ShootData>(
      "/referee/shoot_data",
      10,
      std::bind(&RMSerialDriver::sendShootData, this, std::placeholders::_1)
  );
  robot_status_sub_ = this->create_subscription<rm_referee_ros2::msg::RobotStatus>(
      "/referee/robot_status",
      10,
      std::bind(&RMSerialDriver::sendRobotStatus, this, std::placeholders::_1)
  );
}

RMSerialDriver::~RMSerialDriver() {
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

/**
 * @brief 接收视觉与下位机数据的线程函数
 * @details 采用状态机逻辑：同步帧头 -> 读取定长负载 -> CRC校验 -> 业务处理。
 * 使用逐字节读取策略以应对串口通信中的粘包或断包情况。
 */
void RMSerialDriver::receiveDataVision() {
  // 预分配内存，避免在 while 循环中频繁申请释放带来的开销
  std::vector<uint8_t> header(1);
  std::vector<uint8_t> data;
  data.reserve(sizeof(ReceivePacketVision));

  while (rclcpp::ok()) {
    try {
      // --- 阶段一：帧头同步 (Frame Header Sync) ---
      // 持续读取单字节，直到匹配帧头 0x5A
      while (rclcpp::ok()) {
        serial_driver_->port()->receive(header);

        if (header.empty()) {
          continue;  // 读空，重试
        }

        if (header[0] == 0x5A) {
          break;  // 成功匹配帧头
        }
      }

      // --- 阶段二：读取负载 (Read Payload) ---
      // 目标：读取 ReceivePacketVision 结构体大小的完整数据
      // 策略：逐字节阻塞读取，直到填满 buffer，防止 read 返回部分数据导致解析错误

      data.clear();
      data.push_back(header[0]);  // 放入已匹配的帧头 0x5A

      size_t target_size = sizeof(ReceivePacketVision);

      while (data.size() < target_size && rclcpp::ok()) {
        std::vector<uint8_t> byte_buf(1);
        try {
          serial_driver_->port()->receive(byte_buf);
          data.push_back(byte_buf[0]);
        } catch (const std::exception& ex) {
          RCLCPP_WARN(get_logger(), "Serial receive timeout or error: %s", ex.what());
          break;  // 异常中断，交由外层处理或重置同步
        }
      }

      // 若读取未完成（如中途异常），丢弃本帧，重新寻找帧头
      if (data.size() < target_size) {
        continue;
      }

      // --- 阶段三：反序列化与校验 (Deserialize & Check) ---
      ReceivePacketVision packet = fromVector<ReceivePacketVision>(data);

      bool crc_ok =
          crc16::Verify_CRC16_Check_Sum(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));

      if (crc_ok) {
        // Debug: 打印接收到的原始 HEX 数据 (限速 100ms 一次)
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0');
        for (const auto& byte : data) {
          ss << std::setw(2) << static_cast<int>(byte) << " ";
        }
        RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 100, "Receive packet successfully!");
        RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 100, "Data HEX: %s", ss.str().c_str());

        // --- 阶段四：业务逻辑处理 ---

        // 1. 处理颜色切换逻辑
        uint8_t detect_color = packet.flags & 0x01;
        if (!initial_set_param_ || detect_color != previous_receive_color_) {
          setParam(rclcpp::Parameter("detect_color", detect_color));
          previous_receive_color_ = detect_color;
        }

        // 2. 处理重置 Tracker 请求
        if (packet.flags & 0x02) {
          resetTracker();
        }

        // 3. 发布 TF 变换
        geometry_msgs::msg::TransformStamped t;
        timestamp_offset_ = this->get_parameter("timestamp_offset").as_double();
        t.header.stamp = this->now() + rclcpp::Duration::from_seconds(timestamp_offset_);
        t.header.frame_id = "odom";
        t.child_frame_id = "gimbal_link";
        tf2::Quaternion q;
        q.setRPY(packet.roll, packet.pitch, packet.yaw);
        t.transform.rotation = tf2::toMsg(q);
        tf_broadcaster_->sendTransform(t);

        // 4. 发布调试用瞄准点 (Aiming Point)
        if (abs(packet.aim_x) > 0.01) {
          aiming_point_.header.stamp = this->now();
          aiming_point_.pose.position.x = packet.aim_x;
          aiming_point_.pose.position.y = packet.aim_y;
          aiming_point_.pose.position.z = packet.aim_z;
          marker_pub_->publish(aiming_point_);
        }
      } else {
        // CRC 校验失败
        RCLCPP_ERROR(get_logger(), "CRC error!");
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0');
        for (const auto& byte : data) {
          ss << std::setw(2) << static_cast<int>(byte) << " ";
        }
        RCLCPP_ERROR(get_logger(), "CRC Fail Data: %s", ss.str().c_str());
      }

    } catch (const std::exception& ex) {
      // 严重错误（如设备断开），尝试重连
      RCLCPP_ERROR_THROTTLE(
          get_logger(), *get_clock(), 20, "Error while receiving data: %s", ex.what()
      );
      reopenPort();
    }
  }
}

/**
 * @brief 发送自瞄视觉解算数据
 * @param msg 包含目标位置、速度的 Target 消息
 */
void RMSerialDriver::sendDataVision(const auto_aim_interfaces::msg::Target::SharedPtr msg) {
  const static std::map<std::string, uint8_t> ID_UNIT8_MAP{
      {"", 0},
      {"outpost", 0},
      {"1", 1},
      {"1", 1},
      {"2", 2},
      {"3", 3},
      {"4", 4},
      {"5", 5},
      {"guard", 6},
      {"base", 7}
  };

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
    crc16::Append_CRC16_Check_Sum(reinterpret_cast<uint8_t*>(&packet), sizeof(packet));

    std::vector<uint8_t> data = toVector(packet);

    serial_driver_->port()->send(data);

    // 计算并发布延迟
    std_msgs::msg::Float64 latency;
    latency.data = (this->now() - msg->header.stamp).seconds() * 1000.0;
    RCLCPP_DEBUG_STREAM(get_logger(), "Total latency: " + std::to_string(latency.data) + "ms");
    latency_pub_->publish(latency);
  } catch (const std::exception& ex) {
    RCLCPP_ERROR(get_logger(), "Error while sending data: %s", ex.what());
    reopenPort();
  }
}

/**
 * @brief 发送底盘控制指令 (Twist)
 * @param msg 包含线速度和角速度的 Twist 消息
 */
void RMSerialDriver::sendDataTwist(const geometry_msgs::msg::Twist::SharedPtr msg) {
  try {
    SendPacketTwist packet;
    packet.linear_x = msg->linear.x;
    packet.linear_y = msg->linear.y;
    packet.linear_z = msg->linear.z;
    packet.angular_x = msg->angular.x;
    packet.angular_y = msg->angular.y;
    packet.angular_z = msg->angular.z;
    crc16::Append_CRC16_Check_Sum(reinterpret_cast<uint8_t*>(&packet), sizeof(packet));

    std::vector<uint8_t> data = toVector(packet);

    serial_driver_->port()->send(data);

  } catch (const std::exception& ex) {
    RCLCPP_ERROR(get_logger(), "Error while sending data: %s", ex.what());
    reopenPort();
  }
}

/**
 * @brief 发送射击状态数据
 * @param msg 裁判系统射击数据
 */
void RMSerialDriver::sendShootData(const rm_referee_ros2::msg::ShootData::SharedPtr msg) {
  try {
    ShootDataPacket packet;  // 创建数据包结构体

    // 复制数据
    packet.bullet_type = msg->bullet_type;
    packet.shooter_id = msg->shooter_id;
    packet.bullet_freq = msg->bullet_freq;
    packet.bullet_speed = msg->bullet_speed;

    // 计算 CRC 校验和
    crc16::Append_CRC16_Check_Sum(reinterpret_cast<uint8_t*>(&packet), sizeof(packet));

    std::vector<uint8_t> data = toVector(packet);

    serial_driver_->port()->send(data);

  } catch (const std::exception& ex) {
    RCLCPP_ERROR(get_logger(), "Error while sending shoot data: %s", ex.what());
    reopenPort();  // 异常处理和重启串口
  }
}

/**
 * @brief 发送机器人自身状态数据（血量、功率限制等）
 * @param msg 裁判系统机器人状态数据
 */
void RMSerialDriver::sendRobotStatus(const rm_referee_ros2::msg::RobotStatus::SharedPtr msg) {
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

    // 将布尔类型的电源输出状态压缩到位掩码 (Bitmask) 中
    // Bit 0: Gimbal, Bit 1: Chassis, Bit 2: Shooter
    packet.power_management = 0x00;  // 初始化
    if (msg->power_management_gimbal_output) packet.power_management |= (1 << 0);
    if (msg->power_management_chassis_output) packet.power_management |= (1 << 1);
    if (msg->power_management_shooter_output) packet.power_management |= (1 << 2);

    // 计算 CRC 校验和
    crc16::Append_CRC16_Check_Sum(reinterpret_cast<uint8_t*>(&packet), sizeof(packet));

    std::vector<uint8_t> data = toVector(packet);

    serial_driver_->port()->send(data);

  } catch (const std::exception& ex) {
    RCLCPP_ERROR(get_logger(), "Error while sending robot status: %s", ex.what());
    reopenPort();  // 异常处理和重启串口
  }
}

/**
 * @brief 加载并校验 ROS 参数
 */
void RMSerialDriver::getParams() {
  using FlowControl = drivers::serial_driver::FlowControl;
  using Parity = drivers::serial_driver::Parity;
  using StopBits = drivers::serial_driver::StopBits;

  uint32_t baud_rate{};
  auto fc = FlowControl::NONE;
  auto pt = Parity::NONE;
  auto sb = StopBits::ONE;

  try {
    device_name_ = declare_parameter<std::string>("device_name", "");
  } catch (rclcpp::ParameterTypeException& ex) {
    RCLCPP_ERROR(get_logger(), "The device name provided was invalid");
    throw ex;
  }

  try {
    baud_rate = declare_parameter<int>("baud_rate", 0);
  } catch (rclcpp::ParameterTypeException& ex) {
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
          "The flow_control parameter must be one of: none, software, or hardware."
      };
    }
  } catch (rclcpp::ParameterTypeException& ex) {
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
  } catch (rclcpp::ParameterTypeException& ex) {
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
  } catch (rclcpp::ParameterTypeException& ex) {
    RCLCPP_ERROR(get_logger(), "The stop_bits provided was invalid");
    throw ex;
  }

  device_config_ =
      std::make_unique<drivers::serial_driver::SerialPortConfig>(baud_rate, fc, pt, sb);
}

/**
 * @brief 尝试重启串口
 * @details 当串口发生异常断开时调用，会无限尝试重连直到成功。
 */
void RMSerialDriver::reopenPort() {
  RCLCPP_WARN(get_logger(), "Attempting to reopen port");
  try {
    if (serial_driver_->port()->is_open()) {
      serial_driver_->port()->close();
    }
    serial_driver_->port()->open();
    RCLCPP_INFO(get_logger(), "Successfully reopened port");
  } catch (const std::exception& ex) {
    RCLCPP_ERROR(get_logger(), "Error while reopening port: %s", ex.what());
    if (rclcpp::ok()) {
      rclcpp::sleep_for(std::chrono::seconds(1));
      reopenPort();
    }
  }
}

/**
 * @brief 设置外部节点（检测器）的参数
 * @param param 需要设置的 ROS 参数
 */
void RMSerialDriver::setParam(const rclcpp::Parameter& param) {
  // 设置 armor_detector 参数
  if (!detector_param_client_->service_is_ready()) {
    RCLCPP_WARN(get_logger(), "Armor service not ready, skipping parameter set");
    return;
  }

  if (!set_param_future_.valid() ||
      set_param_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    RCLCPP_INFO(get_logger(), "Setting armor detect_color to %ld...", param.as_int());

    set_param_future_ = detector_param_client_->set_parameters(
        {param}, [this, param](const ResultFuturePtr& results) {
          for (const auto& result : results.get()) {
            if (!result.successful) {
              RCLCPP_ERROR(get_logger(), "Failed to set parameter: %s", result.reason.c_str());
              return;
            }
          }
          RCLCPP_INFO(get_logger(), "Successfully set armor detect_color to %ld!", param.as_int());
          initial_set_param_ = true;
        }
    );
  }

  // 设置 rm_rune_detector 参数
  if (!rune_detector_param_client_->service_is_ready()) {
    RCLCPP_WARN(get_logger(), "Rune service not ready, skipping parameter set");
    return;
  }

  if (!set_rune_detector_param_future_.valid() ||
      set_rune_detector_param_future_.wait_for(std::chrono::seconds(0)) ==
          std::future_status::ready) {
    RCLCPP_INFO(get_logger(), "Setting rune detect_color to %ld...", param.as_int());
    set_rune_detector_param_future_ = rune_detector_param_client_->set_parameters(
        {param}, [this, param](const ResultFuturePtr& results) {
          for (const auto& result : results.get()) {
            if (!result.successful) {
              RCLCPP_ERROR(get_logger(), "Failed to set parameter: %s", result.reason.c_str());
              return;
            }
          }
          RCLCPP_INFO(
              get_logger(), "Successfully set rune detect_color to %ld!", 1 - param.as_int()
          );
        }
    );
  }
}

void RMSerialDriver::resetTracker() {
  if (!reset_tracker_client_->service_is_ready()) {
    RCLCPP_WARN(get_logger(), "Service not ready, skipping tracker reset");
    return;
  }

  auto request = std::make_shared<std_srvs::srv::Trigger::Request>();
  reset_tracker_client_->async_send_request(request);
  RCLCPP_INFO(get_logger(), "Reset tracker!");
}

}  // namespace rm_serial_driver

// Register Node Macro
#include "rclcpp_components/register_node_macro.hpp"

// Register the component with class_loader.
// This acts as a sort of entry point, allowing the component to be discoverable when its library
// is being loaded into a running process.
RCLCPP_COMPONENTS_REGISTER_NODE(rm_serial_driver::RMSerialDriver)

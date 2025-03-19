#include <rclcpp/rclcpp.hpp>
#include <QApplication>
#include <memory>
#include "fake_serial_ros2/fake_serial.hpp"
#include "fake_serial_ros2/main_window.hpp"

int main(int argc, char* argv[])
{
  // 初始化ROS2
  rclcpp::init(argc, argv);
  
  // 创建Qt应用程序
  QApplication app(argc, argv);
  app.setApplicationName("Fake Serial Tool");
  
  // 创建ROS2节点
  auto fake_serial = std::make_shared<fake_serial_ros2::FakeSerial>();
  
  // 创建Qt主窗口
  fake_serial_ros2::MainWindow main_window(fake_serial);
  main_window.show();
  
  // 创建ROS2执行器
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(fake_serial);
  
  // 创建ROS2执行线程
  std::thread ros_thread([&executor]() {
    executor.spin();
  });
  
  // 运行Qt事件循环
  int result = app.exec();
  
  // 清理资源
  rclcpp::shutdown();
  if (ros_thread.joinable()) {
    ros_thread.join();
  }
  
  return result;
} 
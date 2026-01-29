#include <memory>
#include <rclcpp/rclcpp.hpp>
#include "rm_decision/decision_system.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  
  auto node = std::make_shared<rm_decision::DecisionSystem>();
  rclcpp::spin(node);
  
  rclcpp::shutdown();
  return 0;
} 
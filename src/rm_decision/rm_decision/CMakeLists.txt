cmake_minimum_required(VERSION 3.8)
project(rm_decision)

# 设置C++标准为C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 查找依赖的包
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(rm_referee_ros2 REQUIRED)

# 包含头文件目录
include_directories(
  include
  ${YAML_CPP_INCLUDE_DIR}
)

# 添加可执行文件
add_executable(decision_node 
  src/decision_node.cpp
  src/decision_system.cpp
  src/strategy_manager.cpp
)

# 设置目标属性
ament_target_dependencies(decision_node
  rclcpp
  std_msgs
  geometry_msgs
  nav_msgs
  builtin_interfaces
  rm_referee_ros2
)

target_link_libraries(decision_node
  ${YAML_CPP_LIBRARIES}
)

# 安装可执行文件
install(TARGETS
  decision_node
  DESTINATION lib/${PROJECT_NAME}
)

# 安装头文件
install(DIRECTORY
  include/
  DESTINATION include
)

# 安装配置文件
install(DIRECTORY
  config/
  DESTINATION share/${PROJECT_NAME}/config
)

# 安装启动文件
install(DIRECTORY
  launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

# 测试
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package() 
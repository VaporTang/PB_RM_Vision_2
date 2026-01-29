cmake_minimum_required(VERSION 3.8)
project(rm_decision_tmp)

# 设置C++标准
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

# 编译优化选项
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 查找依赖包
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(rm_referee_ros2 REQUIRED)

# 添加可执行文件
add_executable(simple_navigation_node src/simple_navigation.cpp)
ament_target_dependencies(simple_navigation_node
  rclcpp
  geometry_msgs
  nav_msgs
  sensor_msgs
  tf2
  tf2_ros
  rm_referee_ros2
)

# 安装可执行文件
install(TARGETS
  simple_navigation_node
  DESTINATION lib/${PROJECT_NAME}
)

# 安装启动文件
install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
)

# 安装参数文件
install(DIRECTORY
  config
  DESTINATION share/${PROJECT_NAME}
)

# 导出包依赖
ament_package() 
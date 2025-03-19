cmake_minimum_required(VERSION 3.8)
project(fake_serial_ros2)

# 设置C++标准
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 查找依赖包
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rm_referee_ros2 REQUIRED)

# 设置Qt环境
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 包含头文件目录
include_directories(
  include
  ${CMAKE_CURRENT_BINARY_DIR}
)

# 明确指定需要MOC处理的头文件
set(MOC_HEADERS
  include/fake_serial_ros2/main_window.hpp
)

# 使用Qt5的MOC处理器处理头文件
qt5_wrap_cpp(MOC_SOURCES ${MOC_HEADERS})

# 添加可执行文件
add_executable(fake_serial_node
  src/fake_serial_node.cpp
  src/fake_serial.cpp
  src/main_window.cpp
  ${MOC_SOURCES}
)

# 链接Qt库
target_link_libraries(fake_serial_node
  Qt5::Core
  Qt5::Widgets
)

# 设置链接库
ament_target_dependencies(fake_serial_node
  rclcpp
  std_msgs
  rm_referee_ros2
)

# 安装可执行文件
install(TARGETS
  fake_serial_node
  DESTINATION lib/${PROJECT_NAME}
)

# 安装launch文件
install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
)

# 确保include目录安装
install(DIRECTORY
  include/
  DESTINATION include/
)

ament_package() 
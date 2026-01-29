from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration
from launch.actions import DeclareLaunchArgument
from ament_index_python.packages import get_package_share_directory

import os


def generate_launch_description():
    # 获取包路径
    pkg_dir = get_package_share_directory('rm_decision_tmp')
    config_dir = os.path.join(pkg_dir, 'config')
    
    # 声明参数
    use_sim_time = LaunchConfiguration('use_sim_time', default='false')
    move_distance = LaunchConfiguration('move_distance', default='1.0')
    move_time_factor = LaunchConfiguration('move_time_factor', default='5.0')
    
    # 声明参数
    return LaunchDescription([
        # 声明参数
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='false',
            description='使用仿真时间'),
            
        DeclareLaunchArgument(
            'move_distance',
            default_value='1.0',
            description='比赛开始后前进的距离（米）'),
            
        DeclareLaunchArgument(
            'move_time_factor',
            default_value='5.0',
            description='距离到时间的转换因子（秒/米）'),
        
        # 简单导航节点
        Node(
            package='rm_decision_tmp',
            executable='simple_navigation',
            name='simple_navigation',
            output='screen',
            parameters=[
                {'use_sim_time': use_sim_time},
                {'cmd_vel_topic': '/cmd_vel_chassis'},
                {'game_status_topic': '/referee/game_status'},
                {'linear_speed': 0.2},
                {'control_frequency': 10.0},
                {'move_distance': move_distance},
                {'move_time_factor': move_time_factor}
            ]
        )
    ]) 

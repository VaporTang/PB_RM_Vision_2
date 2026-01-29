import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    # 获取包路径
    pkg_dir = get_package_share_directory('rm_decision')
    
    # 配置文件路径
    default_config_path = os.path.join(pkg_dir, 'config', 'strategies.yaml')
    
    # 声明参数
    config_path_arg = DeclareLaunchArgument(
        'config_path',
        default_value=default_config_path,
        description='决策策略配置文件的路径'
    )
    
    frame_id_arg = DeclareLaunchArgument(
        'frame_id',
        default_value='map',
        description='导航点使用的坐标系'
    )
    
    # 决策节点
    decision_node = Node(
        package='rm_decision',
        executable='decision_node',
        name='decision_system',
        output='screen',
        parameters=[{
            'config_path': LaunchConfiguration('config_path'),
            'frame_id': LaunchConfiguration('frame_id')
        }]
    )
    
    # 创建并返回启动描述
    return LaunchDescription([
        config_path_arg,
        frame_id_arg,
        decision_node
    ]) 
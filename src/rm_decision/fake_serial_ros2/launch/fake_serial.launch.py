from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='fake_serial_ros2',
            executable='fake_serial_node',
            name='fake_serial',
            output='screen',
            emulate_tty=True,
        )
    ]) 
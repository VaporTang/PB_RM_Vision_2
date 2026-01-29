from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration
from launch.actions import DeclareLaunchArgument

def generate_launch_description():
    # 声明Launch参数
    use_amcl = LaunchConfiguration('use_amcl')
    use_amcl_arg = DeclareLaunchArgument(
        'use_amcl',
        default_value='true',
        description='Whether to use AMCL pose (true) or odom (false) for position'
    )
    
    initial_hp = LaunchConfiguration('initial_hp')
    initial_hp_arg = DeclareLaunchArgument(
        'initial_hp',
        default_value='500',
        description='Initial health points for the robot'
    )
    
    initial_bullets = LaunchConfiguration('initial_bullets')
    initial_bullets_arg = DeclareLaunchArgument(
        'initial_bullets',
        default_value='300',
        description='Initial bullet count for the robot'
    )
    
    start_delay = LaunchConfiguration('start_delay')
    start_delay_arg = DeclareLaunchArgument(
        'start_delay',
        default_value='10.0',
        description='Delay in seconds before starting navigation'
    )
    
    # 创建模拟裁判系统节点
    mock_referee_node = Node(
        package='rm_decision_tmp',
        executable='mock_referee_system',
        name='mock_referee_system',
        output='screen',
        parameters=[{
            'use_amcl': use_amcl,
            'initial_hp': initial_hp,
            'initial_bullets': initial_bullets
        }]
    )
    
    # 创建固定点导航测试节点
    fixed_nav_node = Node(
        package='rm_decision_tmp',
        executable='fixed_point_navigation',
        name='fixed_point_navigation',
        output='screen',
        parameters=[{
            'start_delay': start_delay
        }]
    )
    
    # 创建运行simple_navigation节点
    simple_nav_node = Node(
        package='rm_decision_tmp',
        executable='simple_navigation',
        name='simple_navigation',
        output='screen'
    )
    
    return LaunchDescription([
        # 声明参数
        use_amcl_arg,
        initial_hp_arg,
        initial_bullets_arg,
        start_delay_arg,
        
        # 启动节点
        mock_referee_node,
        fixed_nav_node,
        simple_nav_node
    ]) 
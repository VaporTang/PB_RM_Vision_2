#ifndef RM_DECISION_STRATEGY_MANAGER_HPP_
#define RM_DECISION_STRATEGY_MANAGER_HPP_

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <yaml-cpp/yaml.h>

namespace rm_decision
{

/**
 * @brief 策略类型枚举
 */
enum class StrategyType {
  OFFENSIVE,  // 进攻策略
  DEFENSIVE   // 撤退策略
};

/**
 * @brief 导航点数据结构
 */
struct NavPoint {
  geometry_msgs::msg::PoseStamped pose;
  double delay;  // 到达后的延迟时间
};

/**
 * @brief 策略数据结构
 */
struct Strategy {
  std::string name;
  std::vector<NavPoint> nav_points;
  double recovery_delay;  // 撤退策略的恢复延迟
  double loop_delay;      // 循环延迟（仅适用于进攻策略）
};

/**
 * @brief 策略管理器类，负责加载和管理不同的策略
 */
class StrategyManager
{
public:
  /**
   * @brief 构造函数
   * @param config_path 配置文件路径
   */
  explicit StrategyManager(const std::string & config_path);

  /**
   * @brief 获取指定类型的策略
   * @param type 策略类型
   * @param strategy_name 策略名称，默认使用配置文件中指定的默认策略
   * @return 策略对象
   */
  Strategy getStrategy(
    StrategyType type,
    const std::string & strategy_name = "") const;

  /**
   * @brief 获取配置的机器人ID
   * @return 机器人ID
   */
  int getRobotId() const {return robot_id_;}

  /**
   * @brief 获取配置的血量阈值
   * @return 血量阈值
   */
  uint16_t getHpThreshold() const {return hp_threshold_;}

  /**
   * @brief 获取配置的子弹阈值
   * @return 子弹阈值
   */
  uint16_t getBulletThreshold() const {return bullet_threshold_;}

  /**
   * @brief 获取开始延迟时间
   * @return 开始延迟时间
   */
  double getStartDelay() const {return start_delay_;}

private:
  /**
   * @brief 加载配置文件
   * @param config_path 配置文件路径
   */
  void loadConfig(const std::string & config_path);
  
  /**
   * @brief 从YAML节点加载导航点
   * @param node YAML节点
   * @param index 导航点索引
   * @return 导航点
   */
  NavPoint loadNavPoint(const YAML::Node & node, int index);

  // 策略集合，按类型和名称组织
  std::map<StrategyType, std::map<std::string, Strategy>> strategies_;
  
  // 默认策略名称
  std::string default_offensive_strategy_;
  std::string default_defensive_strategy_;
  
  // 配置参数
  int robot_id_;
  uint16_t hp_threshold_;
  uint16_t bullet_threshold_;
  double start_delay_;
};

}  // namespace rm_decision

#endif  // RM_DECISION_STRATEGY_MANAGER_HPP_ 
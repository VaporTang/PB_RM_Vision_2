#include "rm_decision/strategy_manager.hpp"

#include <iostream>
#include <fstream>
#include <stdexcept>

namespace rm_decision
{

StrategyManager::StrategyManager(const std::string & config_path)
: strategies_(),
  default_offensive_strategy_("strategy_1"),
  default_defensive_strategy_("strategy_1"),
  robot_id_(7),
  hp_threshold_(300),
  bullet_threshold_(50),
  start_delay_(3.0)
{
  loadConfig(config_path);
}

Strategy StrategyManager::getStrategy(
  StrategyType type,
  const std::string & strategy_name) const
{
  std::string name = strategy_name;
  
  // 如果未指定策略名称，使用默认策略
  if (name.empty()) {
    name = (type == StrategyType::OFFENSIVE) ?
      default_offensive_strategy_ : default_defensive_strategy_;
  }
  
  // 查找策略
  auto type_it = strategies_.find(type);
  if (type_it == strategies_.end()) {
    throw std::runtime_error("Strategy type not found");
  }
  
  auto strategy_it = type_it->second.find(name);
  if (strategy_it == type_it->second.end()) {
    throw std::runtime_error("Strategy name not found: " + name);
  }
  
  return strategy_it->second;
}

void StrategyManager::loadConfig(const std::string & config_path)
{
  try {
    YAML::Node config = YAML::LoadFile(config_path);
    
    // 读取基本配置
    if (config["decision"]) {
      YAML::Node decision = config["decision"];
      
      if (decision["robot_id"]) {
        robot_id_ = decision["robot_id"].as<int>();
      }
      
      if (decision["hp_threshold"]) {
        hp_threshold_ = decision["hp_threshold"].as<uint16_t>();
      }
      
      if (decision["bullet_threshold"]) {
        bullet_threshold_ = decision["bullet_threshold"].as<uint16_t>();
      }
      
      if (decision["start_delay"]) {
        start_delay_ = decision["start_delay"].as<double>();
      }
      
      if (decision["default_offensive_strategy"]) {
        default_offensive_strategy_ = decision["default_offensive_strategy"].as<std::string>();
      }
      
      if (decision["default_defensive_strategy"]) {
        default_defensive_strategy_ = decision["default_defensive_strategy"].as<std::string>();
      }
      
      // 加载进攻策略
      if (decision["offensive_strategies"]) {
        YAML::Node offensive = decision["offensive_strategies"];
        for (const auto & strat_pair : offensive) {
          std::string name = strat_pair.first.as<std::string>();
          YAML::Node strat_node = strat_pair.second;
          
          Strategy strategy;
          strategy.name = name;
          
          // 读取导航点1-4
          for (int i = 1; i <= 4; ++i) {
            std::string point_key = "nav_point_" + std::to_string(i);
            std::string delay_key = "delay_" + std::to_string(i);
            
            if (strat_node[point_key]) {
              NavPoint nav_point = loadNavPoint(strat_node, i);
              strategy.nav_points.push_back(nav_point);
            }
          }
          
          // 读取循环延迟
          if (strat_node["loop_delay"]) {
            strategy.loop_delay = strat_node["loop_delay"].as<double>();
          } else {
            strategy.loop_delay = 3.0;  // 默认值
          }
          
          // 添加到策略集合
          strategies_[StrategyType::OFFENSIVE][name] = strategy;
        }
      }
      
      // 加载撤退策略
      if (decision["defensive_strategies"]) {
        YAML::Node defensive = decision["defensive_strategies"];
        for (const auto & strat_pair : defensive) {
          std::string name = strat_pair.first.as<std::string>();
          YAML::Node strat_node = strat_pair.second;
          
          Strategy strategy;
          strategy.name = name;
          
          // 读取导航点1-2
          for (int i = 1; i <= 2; ++i) {
            std::string point_key = "nav_point_" + std::to_string(i);
            std::string delay_key = "delay_" + std::to_string(i);
            
            if (strat_node[point_key]) {
              NavPoint nav_point = loadNavPoint(strat_node, i);
              strategy.nav_points.push_back(nav_point);
            }
          }
          
          // 读取恢复延迟
          if (strat_node["recovery_delay"]) {
            strategy.recovery_delay = strat_node["recovery_delay"].as<double>();
          } else {
            strategy.recovery_delay = 10.0;  // 默认值
          }
          
          // 添加到策略集合
          strategies_[StrategyType::DEFENSIVE][name] = strategy;
        }
      }
    }
  } catch (const std::exception & e) {
    std::cerr << "Error loading config file: " << e.what() << std::endl;
    throw;
  }
}

NavPoint StrategyManager::loadNavPoint(const YAML::Node & node, int index)
{
  NavPoint nav_point;
  std::string point_key = "nav_point_" + std::to_string(index);
  std::string delay_key = "delay_" + std::to_string(index);
  
  // 读取位置
  if (node[point_key]) {
    auto pose_array = node[point_key].as<std::vector<double>>();
    if (pose_array.size() >= 7) {
      nav_point.pose.pose.position.x = pose_array[0];
      nav_point.pose.pose.position.y = pose_array[1];
      nav_point.pose.pose.position.z = pose_array[2];
      nav_point.pose.pose.orientation.x = pose_array[3];
      nav_point.pose.pose.orientation.y = pose_array[4];
      nav_point.pose.pose.orientation.z = pose_array[5];
      nav_point.pose.pose.orientation.w = pose_array[6];
    }
  }
  
  // 读取延迟
  if (node[delay_key]) {
    nav_point.delay = node[delay_key].as<double>();
  } else {
    nav_point.delay = 2.0;  // 默认延迟
  }
  
  return nav_point;
}

}  // namespace rm_decision 
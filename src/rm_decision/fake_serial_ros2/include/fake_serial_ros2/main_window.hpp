#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QTimer>
#include <QMessageBox>

#include "fake_serial_ros2/fake_serial.hpp"

namespace fake_serial_ros2
{

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(std::shared_ptr<FakeSerial> fake_serial, QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  // 游戏状态相关槽
  void updateGameStatus();
  
  // 机器人血量相关槽
  void updateGameRobotHp();
  
  // 游戏结果相关槽
  void updateGameResult();
  
  // 场地事件相关槽
  void updateFieldEvents();
  
  // 机器人伤害相关槽
  void updateRobotHurt();
  
  // 裁判警告相关槽
  void updateRefereeWarning();
  
  // 射击数据相关槽
  void updateShootData();
  
  // 子弹剩余相关槽
  void updateBulletRemaining();
  
  // 机器人状态相关槽
  void updateRobotStatus();
  
  // 机器人位置相关槽
  void updateRobotPos();
  
  // 自动发布相关槽
  void toggleAutoPublish(bool enabled);
  
private:
  void setupUi();
  
  // 创建不同功能的标签页
  QWidget* createGameStatusTab();
  QWidget* createGameRobotHpTab();
  QWidget* createGameResultTab();
  QWidget* createFieldEventsTab();
  QWidget* createRobotHurtTab();
  QWidget* createRefereeWarningTab();
  QWidget* createShootDataTab();
  QWidget* createBulletRemainingTab();
  QWidget* createRobotStatusTab();
  QWidget* createRobotPosTab();
  
  // 节点实例
  std::shared_ptr<FakeSerial> fake_serial_;
  
  // 主要UI组件
  QTabWidget* tab_widget_;
  QPushButton* publish_all_btn_;
  QCheckBox* auto_publish_cb_;
  QSpinBox* auto_publish_interval_sb_;
  QTimer* auto_publish_timer_;
  
  // 游戏状态标签页的控件
  QComboBox* game_type_cb_;
  QComboBox* game_progress_cb_;
  QSpinBox* stage_remain_time_sb_;
  
  // 机器人血量标签页的控件
  std::vector<QSpinBox*> red_robot_hp_sb_;
  std::vector<QSpinBox*> blue_robot_hp_sb_;
  QSpinBox* red_outpost_hp_sb_;
  QSpinBox* red_base_hp_sb_;
  QSpinBox* blue_outpost_hp_sb_;
  QSpinBox* blue_base_hp_sb_;
  
  // 游戏结果标签页的控件
  QComboBox* game_result_cb_;
  
  // 场地事件标签页的控件
  QCheckBox* field_events_cb_[32];  // 32位，每位代表一种场地事件
  
  // 机器人伤害标签页的控件
  QComboBox* armor_id_cb_;
  QComboBox* hurt_type_cb_;
  
  // 裁判警告标签页的控件
  QComboBox* warning_level_cb_;
  QSpinBox* foul_robot_id_sb_;
  QSpinBox* warning_count_sb_;
  
  // 射击数据标签页的控件
  QComboBox* bullet_type_cb_;
  QSpinBox* shooter_id_sb_;
  QSpinBox* bullet_freq_sb_;
  QDoubleSpinBox* bullet_speed_dsb_;
  
  // 子弹剩余标签页的控件
  QSpinBox* bullet_17mm_sb_;
  QSpinBox* bullet_42mm_sb_;
  QSpinBox* coin_remaining_sb_;
  
  // 机器人状态标签页的控件
  QSpinBox* robot_id_sb_;
  QSpinBox* robot_level_sb_;
  QSpinBox* current_hp_sb_;
  QSpinBox* maximum_hp_sb_;
  QSpinBox* shooter_cooling_sb_;
  QSpinBox* shooter_heat_limit_sb_;
  QSpinBox* chassis_power_limit_sb_;
  QCheckBox* gimbal_output_cb_;
  QCheckBox* chassis_output_cb_;
  QCheckBox* shooter_output_cb_;
  
  // 机器人位置标签页的控件
  QDoubleSpinBox* pos_x_dsb_;
  QDoubleSpinBox* pos_y_dsb_;
  QDoubleSpinBox* angle_dsb_;
};

} // namespace fake_serial_ros2 
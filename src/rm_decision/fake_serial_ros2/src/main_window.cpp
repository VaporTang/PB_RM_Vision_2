#include "fake_serial_ros2/main_window.hpp"

namespace fake_serial_ros2
{

MainWindow::MainWindow(std::shared_ptr<FakeSerial> fake_serial, QWidget *parent)
: QMainWindow(parent), fake_serial_(fake_serial)
{
  setWindowTitle("裁判系统模拟工具");
  resize(800, 600);
  
  // 初始化UI组件
  setupUi();
  
  // 创建自动发布定时器
  auto_publish_timer_ = new QTimer(this);
  connect(auto_publish_timer_, &QTimer::timeout, this, [this]() {
    updateGameStatus();
    updateGameRobotHp();
    updateGameResult();
    updateFieldEvents();
    updateRobotHurt();
    updateRefereeWarning();
    updateShootData();
    updateBulletRemaining();
    updateRobotStatus();
    updateRobotPos();
  });
}

MainWindow::~MainWindow()
{
  // 停止定时器
  if (auto_publish_timer_->isActive()) {
    auto_publish_timer_->stop();
  }
}

void MainWindow::setupUi()
{
  // 创建中央部件
  QWidget *central_widget = new QWidget(this);
  setCentralWidget(central_widget);
  
  // 创建主布局
  QVBoxLayout *main_layout = new QVBoxLayout(central_widget);
  
  // 创建标签页控件
  tab_widget_ = new QTabWidget(central_widget);
  
  // 添加各功能标签页
  tab_widget_->addTab(createGameStatusTab(), "比赛状态");
  tab_widget_->addTab(createGameRobotHpTab(), "机器人血量");
  tab_widget_->addTab(createGameResultTab(), "比赛结果");
  tab_widget_->addTab(createFieldEventsTab(), "场地事件");
  tab_widget_->addTab(createRobotHurtTab(), "机器人伤害");
  tab_widget_->addTab(createRefereeWarningTab(), "裁判警告");
  tab_widget_->addTab(createShootDataTab(), "射击数据");
  tab_widget_->addTab(createBulletRemainingTab(), "子弹剩余");
  tab_widget_->addTab(createRobotStatusTab(), "机器人状态");
  tab_widget_->addTab(createRobotPosTab(), "机器人位置");
  
  main_layout->addWidget(tab_widget_);
  
  // 创建底部控制面板
  QHBoxLayout *control_layout = new QHBoxLayout();
  
  // 发布所有数据按钮
  publish_all_btn_ = new QPushButton("发布所有数据", central_widget);
  connect(publish_all_btn_, &QPushButton::clicked, this, [this]() {
    updateGameStatus();
    updateGameRobotHp();
    updateGameResult();
    updateFieldEvents();
    updateRobotHurt();
    updateRefereeWarning();
    updateShootData();
    updateBulletRemaining();
    updateRobotStatus();
    updateRobotPos();
  });
  
  // 自动发布选项
  auto_publish_cb_ = new QCheckBox("自动发布", central_widget);
  connect(auto_publish_cb_, &QCheckBox::toggled, this, &MainWindow::toggleAutoPublish);
  
  // 发布间隔设置
  QLabel *interval_label = new QLabel("发布间隔(ms):", central_widget);
  auto_publish_interval_sb_ = new QSpinBox(central_widget);
  auto_publish_interval_sb_->setRange(100, 10000);
  auto_publish_interval_sb_->setValue(1000);
  auto_publish_interval_sb_->setSingleStep(100);
  
  control_layout->addWidget(publish_all_btn_);
  control_layout->addWidget(auto_publish_cb_);
  control_layout->addWidget(interval_label);
  control_layout->addWidget(auto_publish_interval_sb_);
  control_layout->addStretch();
  
  main_layout->addLayout(control_layout);
}

QWidget* MainWindow::createGameStatusTab()
{
  QWidget *tab = new QWidget();
  QFormLayout *layout = new QFormLayout(tab);
  
  // 比赛类型
  game_type_cb_ = new QComboBox(tab);
  game_type_cb_->addItem("未开始", 0);
  game_type_cb_->addItem("正式比赛", 1);
  game_type_cb_->addItem("排位赛", 2);
  game_type_cb_->addItem("热身赛", 3);
  game_type_cb_->addItem("演示模式", 4);
  game_type_cb_->setCurrentIndex(1);  // 默认为正式比赛
  layout->addRow("比赛类型:", game_type_cb_);
  
  // 比赛进度
  game_progress_cb_ = new QComboBox(tab);
  game_progress_cb_->addItem("未开始", 0);
  game_progress_cb_->addItem("准备阶段", 1);
  game_progress_cb_->addItem("自检阶段", 2);
  game_progress_cb_->addItem("倒计时", 3);
  game_progress_cb_->addItem("比赛中", 4);
  game_progress_cb_->addItem("比赛结束", 5);
  game_progress_cb_->setCurrentIndex(4);  // 默认为比赛中
  layout->addRow("比赛进度:", game_progress_cb_);
  
  // 当前阶段剩余时间
  stage_remain_time_sb_ = new QSpinBox(tab);
  stage_remain_time_sb_->setRange(0, 600);  // 0~600秒
  stage_remain_time_sb_->setValue(120);     // 默认120秒
  layout->addRow("剩余时间(秒):", stage_remain_time_sb_);
  
  // 发布按钮
  QPushButton *publish_btn = new QPushButton("发布比赛状态", tab);
  connect(publish_btn, &QPushButton::clicked, this, &MainWindow::updateGameStatus);
  layout->addRow("", publish_btn);
  
  return tab;
}

QWidget* MainWindow::createGameRobotHpTab()
{
  QWidget *tab = new QWidget();
  QVBoxLayout *main_layout = new QVBoxLayout(tab);
  
  // 红方机器人血量
  QGroupBox *red_group = new QGroupBox("红方", tab);
  QFormLayout *red_layout = new QFormLayout(red_group);
  
  // 初始化红方血量SpinBox数组
  red_robot_hp_sb_.resize(6);
  red_robot_hp_sb_[0] = new QSpinBox(tab);
  red_robot_hp_sb_[0]->setRange(0, 1000);
  red_robot_hp_sb_[0]->setValue(300);
  red_layout->addRow("英雄(1):", red_robot_hp_sb_[0]);
  
  red_robot_hp_sb_[1] = new QSpinBox(tab);
  red_robot_hp_sb_[1]->setRange(0, 1000);
  red_robot_hp_sb_[1]->setValue(500);
  red_layout->addRow("工程(2):", red_robot_hp_sb_[1]);
  
  for (int i = 2; i < 5; i++) {
    red_robot_hp_sb_[i] = new QSpinBox(tab);
    red_robot_hp_sb_[i]->setRange(0, 1000);
    red_robot_hp_sb_[i]->setValue(200);
    red_layout->addRow(QString("步兵(%1):").arg(i+1), red_robot_hp_sb_[i]);
  }
  
  red_robot_hp_sb_[5] = new QSpinBox(tab);
  red_robot_hp_sb_[5]->setRange(0, 1000);
  red_robot_hp_sb_[5]->setValue(600);
  red_layout->addRow("哨兵(7):", red_robot_hp_sb_[5]);
  
  red_outpost_hp_sb_ = new QSpinBox(tab);
  red_outpost_hp_sb_->setRange(0, 5000);
  red_outpost_hp_sb_->setValue(1500);
  red_layout->addRow("前哨站:", red_outpost_hp_sb_);
  
  red_base_hp_sb_ = new QSpinBox(tab);
  red_base_hp_sb_->setRange(0, 10000);
  red_base_hp_sb_->setValue(5000);
  red_layout->addRow("基地:", red_base_hp_sb_);
  
  // 蓝方机器人血量
  QGroupBox *blue_group = new QGroupBox("蓝方", tab);
  QFormLayout *blue_layout = new QFormLayout(blue_group);
  
  // 初始化蓝方血量SpinBox数组
  blue_robot_hp_sb_.resize(6);
  blue_robot_hp_sb_[0] = new QSpinBox(tab);
  blue_robot_hp_sb_[0]->setRange(0, 1000);
  blue_robot_hp_sb_[0]->setValue(300);
  blue_layout->addRow("英雄(1):", blue_robot_hp_sb_[0]);
  
  blue_robot_hp_sb_[1] = new QSpinBox(tab);
  blue_robot_hp_sb_[1]->setRange(0, 1000);
  blue_robot_hp_sb_[1]->setValue(500);
  blue_layout->addRow("工程(2):", blue_robot_hp_sb_[1]);
  
  for (int i = 2; i < 5; i++) {
    blue_robot_hp_sb_[i] = new QSpinBox(tab);
    blue_robot_hp_sb_[i]->setRange(0, 1000);
    blue_robot_hp_sb_[i]->setValue(200);
    blue_layout->addRow(QString("步兵(%1):").arg(i+1), blue_robot_hp_sb_[i]);
  }
  
  blue_robot_hp_sb_[5] = new QSpinBox(tab);
  blue_robot_hp_sb_[5]->setRange(0, 1000);
  blue_robot_hp_sb_[5]->setValue(600);
  blue_layout->addRow("哨兵(7):", blue_robot_hp_sb_[5]);
  
  blue_outpost_hp_sb_ = new QSpinBox(tab);
  blue_outpost_hp_sb_->setRange(0, 5000);
  blue_outpost_hp_sb_->setValue(1500);
  blue_layout->addRow("前哨站:", blue_outpost_hp_sb_);
  
  blue_base_hp_sb_ = new QSpinBox(tab);
  blue_base_hp_sb_->setRange(0, 10000);
  blue_base_hp_sb_->setValue(5000);
  blue_layout->addRow("基地:", blue_base_hp_sb_);
  
  // 添加两个分组到布局
  QHBoxLayout *groups_layout = new QHBoxLayout();
  groups_layout->addWidget(red_group);
  groups_layout->addWidget(blue_group);
  main_layout->addLayout(groups_layout);
  
  // 发布按钮
  QPushButton *publish_btn = new QPushButton("发布血量数据", tab);
  connect(publish_btn, &QPushButton::clicked, this, &MainWindow::updateGameRobotHp);
  main_layout->addWidget(publish_btn);
  
  return tab;
}

QWidget* MainWindow::createGameResultTab()
{
  QWidget *tab = new QWidget();
  QFormLayout *layout = new QFormLayout(tab);
  
  // 比赛结果
  game_result_cb_ = new QComboBox(tab);
  game_result_cb_->addItem("平局", 0);
  game_result_cb_->addItem("红方胜利", 1);
  game_result_cb_->addItem("蓝方胜利", 2);
  layout->addRow("比赛结果:", game_result_cb_);
  
  // 发布按钮
  QPushButton *publish_btn = new QPushButton("发布比赛结果", tab);
  connect(publish_btn, &QPushButton::clicked, this, &MainWindow::updateGameResult);
  layout->addRow("", publish_btn);
  
  return tab;
}

QWidget* MainWindow::createFieldEventsTab()
{
  QWidget *tab = new QWidget();
  QVBoxLayout *main_layout = new QVBoxLayout(tab);
  
  // 场地事件(32位)
  QGroupBox *events_group = new QGroupBox("场地事件", tab);
  QGridLayout *grid_layout = new QGridLayout(events_group);
  
  // 创建32个复选框表示32位场地事件
  for (int i = 0; i < 32; i++) {
    field_events_cb_[i] = new QCheckBox(QString("事件位%1").arg(i), events_group);
    grid_layout->addWidget(field_events_cb_[i], i / 4, i % 4);
  }
  
  main_layout->addWidget(events_group);
  
  // 发布按钮
  QPushButton *publish_btn = new QPushButton("发布场地事件", tab);
  connect(publish_btn, &QPushButton::clicked, this, &MainWindow::updateFieldEvents);
  main_layout->addWidget(publish_btn);
  
  return tab;
}

QWidget* MainWindow::createRobotHurtTab()
{
  QWidget *tab = new QWidget();
  QFormLayout *layout = new QFormLayout(tab);
  
  // 装甲板ID
  armor_id_cb_ = new QComboBox(tab);
  armor_id_cb_->addItem("前装甲板", 0);
  armor_id_cb_->addItem("左装甲板", 1);
  armor_id_cb_->addItem("后装甲板", 2);
  armor_id_cb_->addItem("右装甲板", 3);
  armor_id_cb_->addItem("上装甲板", 4);
  armor_id_cb_->addItem("下装甲板", 5);
  layout->addRow("装甲板ID:", armor_id_cb_);
  
  // 伤害类型
  hurt_type_cb_ = new QComboBox(tab);
  hurt_type_cb_->addItem("装甲伤害", 0);
  hurt_type_cb_->addItem("模块掉线", 1);
  hurt_type_cb_->addItem("超射速", 2);
  hurt_type_cb_->addItem("超热量", 3);
  hurt_type_cb_->addItem("超功率", 4);
  hurt_type_cb_->addItem("撞击", 5);
  layout->addRow("伤害类型:", hurt_type_cb_);
  
  // 发布按钮
  QPushButton *publish_btn = new QPushButton("发布伤害数据", tab);
  connect(publish_btn, &QPushButton::clicked, this, &MainWindow::updateRobotHurt);
  layout->addRow("", publish_btn);
  
  return tab;
}

QWidget* MainWindow::createRefereeWarningTab()
{
  QWidget *tab = new QWidget();
  QFormLayout *layout = new QFormLayout(tab);
  
  // 警告等级
  warning_level_cb_ = new QComboBox(tab);
  warning_level_cb_->addItem("无警告", 0);
  warning_level_cb_->addItem("双方黄牌", 1);
  warning_level_cb_->addItem("黄牌", 2);
  warning_level_cb_->addItem("红牌", 3);
  warning_level_cb_->addItem("判负", 4);
  layout->addRow("警告等级:", warning_level_cb_);
  
  // 违规机器人ID
  foul_robot_id_sb_ = new QSpinBox(tab);
  foul_robot_id_sb_->setRange(0, 255);
  foul_robot_id_sb_->setValue(0);
  layout->addRow("违规机器人ID:", foul_robot_id_sb_);
  
  // 违规计数
  warning_count_sb_ = new QSpinBox(tab);
  warning_count_sb_->setRange(0, 255);
  warning_count_sb_->setValue(0);
  layout->addRow("违规计数:", warning_count_sb_);
  
  // 发布按钮
  QPushButton *publish_btn = new QPushButton("发布警告数据", tab);
  connect(publish_btn, &QPushButton::clicked, this, &MainWindow::updateRefereeWarning);
  layout->addRow("", publish_btn);
  
  return tab;
}

QWidget* MainWindow::createShootDataTab()
{
  QWidget *tab = new QWidget();
  QFormLayout *layout = new QFormLayout(tab);
  
  // 子弹类型
  bullet_type_cb_ = new QComboBox(tab);
  bullet_type_cb_->addItem("17mm弹丸", 1);
  bullet_type_cb_->addItem("42mm弹丸", 2);
  layout->addRow("子弹类型:", bullet_type_cb_);
  
  // 发射机构ID
  shooter_id_sb_ = new QSpinBox(tab);
  shooter_id_sb_->setRange(1, 3);
  shooter_id_sb_->setValue(1);
  layout->addRow("发射机构ID:", shooter_id_sb_);
  
  // 射频
  bullet_freq_sb_ = new QSpinBox(tab);
  bullet_freq_sb_->setRange(0, 100);
  bullet_freq_sb_->setValue(10);
  layout->addRow("射频(Hz):", bullet_freq_sb_);
  
  // 射速
  bullet_speed_dsb_ = new QDoubleSpinBox(tab);
  bullet_speed_dsb_->setRange(0.0, 30.0);
  bullet_speed_dsb_->setValue(15.0);
  bullet_speed_dsb_->setSingleStep(0.1);
  layout->addRow("射速(m/s):", bullet_speed_dsb_);
  
  // 发布按钮
  QPushButton *publish_btn = new QPushButton("发布射击数据", tab);
  connect(publish_btn, &QPushButton::clicked, this, &MainWindow::updateShootData);
  layout->addRow("", publish_btn);
  
  return tab;
}

QWidget* MainWindow::createBulletRemainingTab()
{
  QWidget *tab = new QWidget();
  QFormLayout *layout = new QFormLayout(tab);
  
  // 17mm子弹剩余发射数
  bullet_17mm_sb_ = new QSpinBox(tab);
  bullet_17mm_sb_->setRange(0, 1000);
  bullet_17mm_sb_->setValue(100);
  layout->addRow("17mm子弹剩余:", bullet_17mm_sb_);
  
  // 42mm子弹剩余发射数
  bullet_42mm_sb_ = new QSpinBox(tab);
  bullet_42mm_sb_->setRange(0, 100);
  bullet_42mm_sb_->setValue(10);
  layout->addRow("42mm子弹剩余:", bullet_42mm_sb_);
  
  // 剩余金币数
  coin_remaining_sb_ = new QSpinBox(tab);
  coin_remaining_sb_->setRange(0, 500);
  coin_remaining_sb_->setValue(50);
  layout->addRow("剩余金币:", coin_remaining_sb_);
  
  // 发布按钮
  QPushButton *publish_btn = new QPushButton("发布子弹剩余数据", tab);
  connect(publish_btn, &QPushButton::clicked, this, &MainWindow::updateBulletRemaining);
  layout->addRow("", publish_btn);
  
  return tab;
}

QWidget* MainWindow::createRobotStatusTab()
{
  QWidget *tab = new QWidget();
  QFormLayout *layout = new QFormLayout(tab);
  
  // 机器人ID
  robot_id_sb_ = new QSpinBox(tab);
  robot_id_sb_->setRange(1, 20);
  robot_id_sb_->setValue(3);  // 默认为红方步兵3
  layout->addRow("机器人ID:", robot_id_sb_);
  
  // 机器人等级
  robot_level_sb_ = new QSpinBox(tab);
  robot_level_sb_->setRange(1, 3);
  robot_level_sb_->setValue(1);
  layout->addRow("机器人等级:", robot_level_sb_);
  
  // 当前血量
  current_hp_sb_ = new QSpinBox(tab);
  current_hp_sb_->setRange(0, 1000);
  current_hp_sb_->setValue(200);
  layout->addRow("当前血量:", current_hp_sb_);
  
  // 最大血量
  maximum_hp_sb_ = new QSpinBox(tab);
  maximum_hp_sb_->setRange(0, 1000);
  maximum_hp_sb_->setValue(200);
  layout->addRow("最大血量:", maximum_hp_sb_);
  
  // 枪管冷却值
  shooter_cooling_sb_ = new QSpinBox(tab);
  shooter_cooling_sb_->setRange(0, 1000);
  shooter_cooling_sb_->setValue(0);
  layout->addRow("枪管冷却值:", shooter_cooling_sb_);
  
  // 枪管热量上限
  shooter_heat_limit_sb_ = new QSpinBox(tab);
  shooter_heat_limit_sb_->setRange(0, 1000);
  shooter_heat_limit_sb_->setValue(240);
  layout->addRow("枪管热量上限:", shooter_heat_limit_sb_);
  
  // 底盘功率限制
  chassis_power_limit_sb_ = new QSpinBox(tab);
  chassis_power_limit_sb_->setRange(0, 500);
  chassis_power_limit_sb_->setValue(60);
  layout->addRow("底盘功率限制:", chassis_power_limit_sb_);
  
  // 输出状态复选框
  QGroupBox *output_group = new QGroupBox("输出状态", tab);
  QHBoxLayout *output_layout = new QHBoxLayout(output_group);
  
  gimbal_output_cb_ = new QCheckBox("云台输出", output_group);
  gimbal_output_cb_->setChecked(true);
  
  chassis_output_cb_ = new QCheckBox("底盘输出", output_group);
  chassis_output_cb_->setChecked(true);
  
  shooter_output_cb_ = new QCheckBox("射击输出", output_group);
  shooter_output_cb_->setChecked(true);
  
  output_layout->addWidget(gimbal_output_cb_);
  output_layout->addWidget(chassis_output_cb_);
  output_layout->addWidget(shooter_output_cb_);
  
  layout->addRow("", output_group);
  
  // 发布按钮
  QPushButton *publish_btn = new QPushButton("发布机器人状态", tab);
  connect(publish_btn, &QPushButton::clicked, this, &MainWindow::updateRobotStatus);
  layout->addRow("", publish_btn);
  
  return tab;
}

QWidget* MainWindow::createRobotPosTab()
{
  QWidget *tab = new QWidget();
  QFormLayout *layout = new QFormLayout(tab);
  
  // X坐标
  pos_x_dsb_ = new QDoubleSpinBox(tab);
  pos_x_dsb_->setRange(-50.0, 50.0);
  pos_x_dsb_->setValue(0.0);
  pos_x_dsb_->setSingleStep(0.1);
  layout->addRow("X坐标(m):", pos_x_dsb_);
  
  // Y坐标
  pos_y_dsb_ = new QDoubleSpinBox(tab);
  pos_y_dsb_->setRange(-50.0, 50.0);
  pos_y_dsb_->setValue(0.0);
  pos_y_dsb_->setSingleStep(0.1);
  layout->addRow("Y坐标(m):", pos_y_dsb_);
  
  // 角度
  angle_dsb_ = new QDoubleSpinBox(tab);
  angle_dsb_->setRange(-180.0, 180.0);
  angle_dsb_->setValue(0.0);
  angle_dsb_->setSingleStep(1.0);
  layout->addRow("角度(度):", angle_dsb_);
  
  // 发布按钮
  QPushButton *publish_btn = new QPushButton("发布位置数据", tab);
  connect(publish_btn, &QPushButton::clicked, this, &MainWindow::updateRobotPos);
  layout->addRow("", publish_btn);
  
  return tab;
}

// 更新和发布消息的槽函数实现
void MainWindow::updateGameStatus()
{
  rm_referee_ros2::msg::GameStatus msg;
  
  msg.game_type = game_type_cb_->currentData().toUInt();
  msg.game_progress = game_progress_cb_->currentData().toUInt();
  msg.stage_remain_time = stage_remain_time_sb_->value();
  
  fake_serial_->publishGameStatus(msg);
}

void MainWindow::updateGameRobotHp()
{
  rm_referee_ros2::msg::GameRobotHp msg;
  
  msg.red_1_robot_hp = red_robot_hp_sb_[0]->value();
  msg.red_2_robot_hp = red_robot_hp_sb_[1]->value();
  msg.red_3_robot_hp = red_robot_hp_sb_[2]->value();
  msg.red_4_robot_hp = red_robot_hp_sb_[3]->value();
  msg.red_5_robot_hp = red_robot_hp_sb_[4]->value();
  msg.red_7_robot_hp = red_robot_hp_sb_[5]->value();
  msg.red_outpost_hp = red_outpost_hp_sb_->value();
  msg.red_base_hp = red_base_hp_sb_->value();
  
  msg.blue_1_robot_hp = blue_robot_hp_sb_[0]->value();
  msg.blue_2_robot_hp = blue_robot_hp_sb_[1]->value();
  msg.blue_3_robot_hp = blue_robot_hp_sb_[2]->value();
  msg.blue_4_robot_hp = blue_robot_hp_sb_[3]->value();
  msg.blue_5_robot_hp = blue_robot_hp_sb_[4]->value();
  msg.blue_7_robot_hp = blue_robot_hp_sb_[5]->value();
  msg.blue_outpost_hp = blue_outpost_hp_sb_->value();
  msg.blue_base_hp = blue_base_hp_sb_->value();
  
  fake_serial_->publishGameRobotHp(msg);
}

void MainWindow::updateGameResult()
{
  rm_referee_ros2::msg::GameResult msg;
  
  msg.winner = game_result_cb_->currentData().toUInt();
  
  fake_serial_->publishGameResult(msg);
}

void MainWindow::updateFieldEvents()
{
  rm_referee_ros2::msg::FieldEvents msg;
  
  // 计算event_type的值
  uint32_t event_type = 0;
  for (int i = 0; i < 32; i++) {
    if (field_events_cb_[i]->isChecked()) {
      event_type |= (1 << i);
    }
  }
  
  msg.event_type = event_type;
  
  fake_serial_->publishFieldEvents(msg);
}

void MainWindow::updateRobotHurt()
{
  rm_referee_ros2::msg::RobotHurt msg;
  
  msg.armor_id = armor_id_cb_->currentData().toUInt();
  msg.hurt_type = hurt_type_cb_->currentData().toUInt();
  
  fake_serial_->publishRobotHurt(msg);
}

void MainWindow::updateRefereeWarning()
{
  rm_referee_ros2::msg::RefereeWarning msg;
  
  msg.level = warning_level_cb_->currentData().toUInt();
  msg.foul_robot_id = foul_robot_id_sb_->value();
  msg.count = warning_count_sb_->value();
  
  fake_serial_->publishRefereeWarning(msg);
}

void MainWindow::updateShootData()
{
  rm_referee_ros2::msg::ShootData msg;
  
  msg.bullet_type = bullet_type_cb_->currentData().toUInt();
  msg.shooter_id = shooter_id_sb_->value();
  msg.bullet_freq = bullet_freq_sb_->value();
  msg.bullet_speed = bullet_speed_dsb_->value();
  
  fake_serial_->publishShootData(msg);
}

void MainWindow::updateBulletRemaining()
{
  rm_referee_ros2::msg::BulletRemaining msg;
  
  msg.bullet_allowance_num_17_mm = bullet_17mm_sb_->value();
  msg.bullet_allowance_num_42_mm = bullet_42mm_sb_->value();
  msg.coin_remaining_num = coin_remaining_sb_->value();
  
  fake_serial_->publishBulletRemaining(msg);
}

void MainWindow::updateRobotStatus()
{
  rm_referee_ros2::msg::RobotStatus msg;
  
  msg.robot_id = robot_id_sb_->value();
  msg.robot_level = robot_level_sb_->value();
  msg.current_hp = current_hp_sb_->value();
  msg.maximum_hp = maximum_hp_sb_->value();
  msg.shooter_barrel_cooling_value = shooter_cooling_sb_->value();
  msg.shooter_barrel_heat_limit = shooter_heat_limit_sb_->value();
  msg.chassis_power_limit = chassis_power_limit_sb_->value();
  msg.power_management_gimbal_output = gimbal_output_cb_->isChecked() ? 1 : 0;
  msg.power_management_chassis_output = chassis_output_cb_->isChecked() ? 1 : 0;
  msg.power_management_shooter_output = shooter_output_cb_->isChecked() ? 1 : 0;
  
  fake_serial_->publishRobotStatus(msg);
}

void MainWindow::updateRobotPos()
{
  rm_referee_ros2::msg::RobotPos msg;
  
  msg.x = pos_x_dsb_->value();
  msg.y = pos_y_dsb_->value();
  msg.angle = angle_dsb_->value();
  
  fake_serial_->publishRobotPos(msg);
}

void MainWindow::toggleAutoPublish(bool enabled)
{
  if (enabled) {
    // 启动定时器
    auto_publish_timer_->start(auto_publish_interval_sb_->value());
  } else {
    // 停止定时器
    auto_publish_timer_->stop();
  }
}

} // namespace fake_serial_ros2 
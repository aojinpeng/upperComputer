

基于提供的代码结构和项目信息，我将生成一份完整的 README.md 文档。

# Qt5.15.2 轻量级 Modbus TCP 多设备 SCADA 监控系统

## 项目简介

本项目是一个跨平台的工业级小型 SCADA（数据采集与监视控制）系统，专注于多设备 Modbus TCP 集中监控、数据持久化与 Linux 工程化部署。系统采用 Qt 5.15.2 Widgets 开发，提供设备管理、数据采集、告警处理、实时曲线显示等核心功能。

## 技术栈

| 类别 | 技术选型 |
|------|----------|
| 框架 | Qt 5.15.2 Widgets + CMake 3.16+ |
| 编程语言 | C++17 |
| 通信协议 | Modbus TCP、QtNetwork |
| 数据库 | MySQL（历史数据）+ SQLite（本地缓存） |
| 图表可视化 | Qt Charts |

## 功能特性

- **多设备监控**：支持同时监控多个 Modbus TCP 设备
- **心跳检测**：设备心跳保活与断线自动重连
- **数据采集**：定时轮询采集保持寄存器数据
- **告警管理**：上下限告警配置与历史记录存储
- **数据持久化**：MySQL 存储历史数据，SQLite 本地缓存
- **HTTP 上报**：支持数据点 HTTP 上传至远程服务器
- **TCP 服务器**：支持向其他客户端广播实时数据
- **日志系统**：分级日志记录与日志轮转
- **配置文件**：JSON 格式配置文件，支持热重载

## 目录结构

```
modbus-tcp-scada/
├── CMakeLists.txt                 # CMake 构建配置
├── README.md                      # 项目说明文档
├── scripts/
│   └── modbus_simulator.py       # Modbus 模拟器脚本
├── sql/
│   ├── alarm.sql                  # 告警表结构
│   ├── device.sql                  # 设备表结构
│   └── history.sql                 # 历史数据表结构
└── src/
    ├── CMakeLists.txt             # 源码 CMake 配置
    ├── main.cpp                    # 程序入口
    ├── comm/                       # 通信模块
    │   ├── HeartBeat.cpp/h         # 设备心跳检测
    │   ├── HttpClient.cpp/h        # HTTP 客户端
    │   ├── ModbusTcpMaster.cpp/h   # Modbus TCP 主站
    │   └── TcpServer.cpp/h         # TCP 数据广播服务器
    ├── common/                     # 公共模块
    │   ├── Config.cpp/h            # 配置管理
    │   ├── Defines.h               # 类型定义与常量
    │   ├── Logger.cpp/h            # 日志系统
    │   └── Utils.cpp/h             # 工具函数
    ├── db/                         # 数据库模块
    │   ├── MysqlManager.cpp/h      # MySQL 管理器
    │   └── SqliteManager.cpp/h     # SQLite 管理器
    ├── logic/                      # 业务逻辑模块
    │   ├── AlarmManager.cpp/h      # 告警管理
    │   ├── CollectManager.cpp/h    # 数据采集管理
    │   └── DeviceManager.cpp/h     # 设备管理
    └── ui/                         # 界面模块
        ├── AlarmWidget.cpp/h       # 告警窗口
        ├── ChartWidget.cpp/h       # 曲线图表窗口
        ├── DevicePanel.cpp/h       # 设备面板
        ├── MainWindow.cpp/h        # 主窗口
        └── MainWindow.ui           # Qt 界面布局文件
```

## 快速开始

### 环境要求

- Qt 5.15.2 或更高版本
- CMake 3.16+
- C++17 编译器
- MySQL 5.7+（用于历史数据存储）
- SQLite3 开发库

### 构建步骤

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make -j$(nproc)

# 运行
./modbus_tcp_scada
```

### 配置说明

配置文件采用 JSON 格式，主要配置项包括：

```json
{
    "mysql": {
        "host": "localhost",
        "port": 3306,
        "user": "root",
        "password": "password",
        "database": "scada"
    },
    "collect_interval_ms": 1000,
    "heartbeat_interval_ms": 5000,
    "reconnect_interval_ms": 3000,
    "devices": [
        {
            "id": 1,
            "name": "设备1",
            "ip": "192.168.1.100",
            "port": 502,
            "slave_id": 1,
            "start_addr": 0,
            "reg_count": 10,
            "alarm_min": 0,
            "alarm_max": 100
        }
    ]
}
```

### 数据库初始化

执行 SQL 目录下的脚本初始化数据库：

```bash
mysql -u root

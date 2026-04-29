# ====================== 第一步：创建核心静态库（主程序和测试共用） ======================
# 把所有业务代码做成静态库ModbusScadaCore
add_library(ModbusScadaCore STATIC
    # 【注意】这里只放业务源码，不要放main.cpp
    # ====================== UI 层 ======================
    ui/MainWindow.cpp
    ui/MainWindow.h
    ui/MainWindow.ui

    ui/ChartWidget.cpp
    ui/ChartWidget.h
    ui/DevicePanelWidget.cpp
    ui/DevicePanelWidget.h

    # ====================== 业务逻辑层 ======================
    logic/DeviceManager.cpp
    logic/DeviceManager.h
    logic/AlarmManager.cpp
    logic/AlarmManager.h
    logic/CollectManager.cpp
    logic/CollectManager.h

    # ====================== 通信层（包含HttpClient！） ======================
    comm/ModbusTcpMaster.cpp
    comm/ModbusTcpMaster.h
    comm/TcpServer.cpp
    comm/TcpServer.h
    comm/HttpClient.cpp  # 关键：HttpClient的源码在这里！
    comm/HttpClient.h
    comm/HeartBeat.cpp
    comm/HeartBeat.h

    # ====================== 数据持久层 ======================
    db/MysqlManager.cpp
    db/MysqlManager.h
    db/SqliteManager.cpp
    db/SqliteManager.h

    # ====================== 通用工具层 ======================
    common/Logger.cpp
    common/Logger.h
    common/Config.cpp
    common/Config.h
    common/Utils.cpp
    common/Utils.h
    common/Defines.h
    common/Defines.cpp

    # ====================== 对话框 ======================
    ui/LoginDialog.h ui/LoginDialog.cpp ui/LoginDialog.ui
    ui/ConfigDialog.h ui/ConfigDialog.cpp ui/ConfigDialog.ui
    ui/AlarmDialog.h ui/AlarmDialog.cpp ui/AlarmDialog.ui
    ui/DataQueryDialog.h ui/DataQueryDialog.cpp ui/DataQueryDialog.ui
    ui/SimpleHttpTest.h ui/SimpleHttpTest.cpp
    ui/DeviceEditDialog.h ui/DeviceEditDialog.cpp
)

# 给核心库设置头文件目录
target_include_directories(ModbusScadaCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/logic
    ${CMAKE_CURRENT_SOURCE_DIR}/comm
    ${CMAKE_CURRENT_SOURCE_DIR}/db
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# 核心库链接Qt模块
target_link_libraries(ModbusScadaCore PUBLIC
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    Qt5::Sql
    Qt5::Charts
    Qt5::SerialBus
)

# ====================== 第二步：编译主程序 ======================
# 主程序只包含main.cpp，其他代码都链接静态库
add_executable(ModbusTcpScada
    main.cpp  # 只有main.cpp！
)

# 主程序链接核心静态库
target_link_libraries(ModbusTcpScada PRIVATE ModbusScadaCore)

# 拷贝配置文件（和原来一样）
add_custom_command(TARGET ModbusTcpScada POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/config
    $<TARGET_FILE_DIR:ModbusTcpScada>/config
    COMMENT "Copying config files to output directory"
)
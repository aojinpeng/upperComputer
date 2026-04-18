# 手动指定所有源码 + 头文件
add_executable(ModbusTcpScada
    # 程序入口
    main.cpp

    # ====================== UI 层 ======================
    ui/MainWindow.cpp
    ui/MainWindow.h
    ui/MainWindow.ui
    ui/AlarmWidget.cpp
    ui/AlarmWidget.h
    ui/ChartWidget.cpp
    ui/ChartWidget.h
    ui/DevicePanel.cpp
    ui/DevicePanel.h

    # ====================== 业务逻辑层 ======================
    logic/DeviceManager.cpp
    logic/DeviceManager.h
    logic/AlarmManager.cpp
    logic/AlarmManager.h
    logic/CollectManager.cpp
    logic/CollectManager.h

    # ====================== 通信层 ======================
    comm/ModbusTcpMaster.cpp
    comm/ModbusTcpMaster.h
    comm/TcpServer.cpp
    comm/TcpServer.h
    comm/HttpClient.cpp
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

)

# 包含所有头文件目录
target_include_directories(ModbusTcpScada PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/logic
    ${CMAKE_CURRENT_SOURCE_DIR}/comm
    ${CMAKE_CURRENT_SOURCE_DIR}/db
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# 链接 Qt5 核心库
target_link_libraries(ModbusTcpScada PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    Qt5::Sql
    Qt5::Charts
)

# 【关键修复：添加配置文件拷贝，写在目标定义之后】
add_custom_command(TARGET ModbusTcpScada POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/config
    $<TARGET_FILE_DIR:ModbusTcpScada>/config
    COMMENT "Copying config files to output directory"
)
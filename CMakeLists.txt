cmake_minimum_required(VERSION 3.16)
project(ModbusTcpScada VERSION 1.0.0 LANGUAGES CXX)

# 强制C++17标准，禁止扩展
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 自动处理Qt MOC、UIC、RCC（会被子目录继承）
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找Qt5必需模块（去掉版本号，让套件自动选择）
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    Sql
    Charts
)

# 定义全局输出目录（跨平台统一）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 添加子目录（先处理src，创建目标）
add_subdirectory(src)

# 【删除原来的add_custom_command，移动到src/CMakeLists.txt】
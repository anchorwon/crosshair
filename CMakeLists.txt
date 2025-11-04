# CMake 配置文件 - 屏幕准心软件

cmake_minimum_required(VERSION 3.10)
project(Crosshair VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows 特定设置
if(WIN32)
    # 设置为 Windows GUI 应用程序（不显示控制台）
    set(CMAKE_WIN32_EXECUTABLE ON)
    
    # 添加 Unicode 定义
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# 添加可执行文件
add_executable(crosshair WIN32 crosshair.cpp)

# 链接 Windows 库
if(WIN32)
    target_link_libraries(crosshair
        gdiplus
        shell32
        user32
        gdi32
        comctl32
    )
endif()

# 设置输出目录
set_target_properties(crosshair PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 安装规则
install(TARGETS crosshair DESTINATION bin)

# 提示信息
message(STATUS "===================================")
message(STATUS "屏幕准心软件 - C++ 版本")
message(STATUS "===================================")
message(STATUS "C++ 标准: C++${CMAKE_CXX_STANDARD}")
message(STATUS "编译器: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "系统: ${CMAKE_SYSTEM_NAME}")
message(STATUS "===================================")


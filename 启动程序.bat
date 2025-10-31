@echo off
chcp 65001 >nul
echo ====================================
echo     屏幕准心软件 - C++ 版本
echo ====================================
echo.

if not exist crosshair.exe (
    echo [错误] 未找到 crosshair.exe
    echo.
    echo 请先编译程序：
    echo   - 使用 MSVC: 运行 build.bat
    echo   - 使用 MinGW: 运行 build_mingw.bat
    echo   - 使用 CMake: 运行 build_cmake.bat
    echo.
    pause
    exit /b 1
)

echo [信息] 正在启动屏幕准心软件...
start crosshair.exe
echo [信息] 程序已在后台运行
echo [信息] 请查看系统托盘图标
echo.
timeout /t 3 >nul
exit


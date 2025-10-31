@echo off
chcp 65001 >nul
echo ====================================
echo   屏幕准心软件 - CMake 编译脚本
echo ====================================
echo.

REM 检查 CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [错误] 未找到 CMake！
    echo.
    echo 请先安装 CMake: https://cmake.org/download/
    echo.
    pause
    exit /b 1
)

echo [信息] 找到 CMake
echo.

REM 创建构建目录
if not exist build mkdir build
cd build

echo [信息] 配置项目...
cmake ..

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [错误] CMake 配置失败！
    cd ..
    pause
    exit /b 1
)

echo.
echo [信息] 开始编译...
cmake --build . --config Release

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ====================================
    echo    编译成功！
    echo ====================================
    echo.
    
    REM 复制可执行文件到根目录
    if exist bin\Release\crosshair.exe (
        copy /Y bin\Release\crosshair.exe ..\crosshair.exe >nul
        echo 已复制到: crosshair.exe
    ) else if exist bin\crosshair.exe (
        copy /Y bin\crosshair.exe ..\crosshair.exe >nul
        echo 已复制到: crosshair.exe
    ) else if exist Release\crosshair.exe (
        copy /Y Release\crosshair.exe ..\crosshair.exe >nul
        echo 已复制到: crosshair.exe
    ) else if exist crosshair.exe (
        copy /Y crosshair.exe ..\crosshair.exe >nul
        echo 已复制到: crosshair.exe
    )
    
    echo.
    echo 运行方式: 双击 crosshair.exe 或运行 启动程序.bat
    echo.
) else (
    echo.
    echo ====================================
    echo    编译失败！
    echo ====================================
    echo.
)

cd ..
pause


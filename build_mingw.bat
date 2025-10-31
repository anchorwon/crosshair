@echo off
chcp 65001 >nul
echo ====================================
echo  屏幕准心软件 - MinGW/G++ 编译脚本
echo ====================================
echo.

REM 检查 g++ 编译器
where g++ >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [错误] 未找到 g++ 编译器！
    echo.
    echo 请先安装 MinGW-w64 或 TDM-GCC
    echo 下载地址: https://www.mingw-w64.org/
    echo.
    pause
    exit /b 1
)

echo [信息] 找到 g++ 编译器
echo [信息] 开始编译...
echo.

REM 编译命令（使用静态链接，避免依赖 DLL）
g++ -std=c++17 -O2 ^
    -static ^
    -mwindows ^
    crosshair.cpp ^
    -lgdiplus -lshell32 -luser32 -lgdi32 -lcomctl32 ^
    -o crosshair.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ====================================
    echo    编译成功！
    echo ====================================
    echo.
    echo 生成文件: crosshair.exe
    echo 文件大小: 
    dir crosshair.exe | find "crosshair.exe"
    echo.
    echo 运行方式: 双击 crosshair.exe
    echo.
) else (
    echo.
    echo ====================================
    echo    编译失败！
    echo ====================================
    echo.
    echo 请检查错误信息并修复后重试
    echo.
)

pause


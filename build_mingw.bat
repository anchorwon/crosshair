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

REM 关闭正在运行的 crosshair.exe
echo [信息] 检查并关闭正在运行的程序...
taskkill /F /IM crosshair.exe >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [信息] 已关闭正在运行的程序
    timeout /t 1 /nobreak >nul
) else (
    echo [信息] 未发现运行中的程序
)
echo.

echo [信息] 开始编译...
echo.

REM 编译资源文件
echo [信息] 编译资源文件...
if exist logo.ico (
    windres crosshair.rc -o crosshair_res.o 2>nul
    if %ERRORLEVEL% EQU 0 (
        echo [信息] 资源文件编译成功
        set RESOURCE_FILE=crosshair_res.o
    ) else (
        echo [警告] 资源文件编译失败，将使用运行时图标（窗口仍会显示图标）
        set RESOURCE_FILE=
    )
) else (
    echo [警告] 未找到 logo.ico，将使用运行时图标
    set RESOURCE_FILE=
)
echo.

REM 编译命令（使用静态链接，避免依赖 DLL）
g++ -std=c++17 -O2 ^
    -static ^
    -mwindows ^
    crosshair.cpp %RESOURCE_FILE% ^
    -lgdiplus -lshell32 -luser32 -lgdi32 -lcomctl32 -ldwmapi ^
    -o crosshair.exe

REM 保存编译结果
set COMPILE_RESULT=%ERRORLEVEL%

REM 清理资源对象文件
if exist crosshair_res.o del crosshair_res.o >nul 2>nul

REM 检查编译是否成功
if %COMPILE_RESULT% EQU 0 (
    if exist crosshair.exe (
        echo.
        echo ====================================
        echo    编译成功！
        echo ====================================
        echo.
        echo 生成文件: crosshair.exe
        echo 文件大小: 
        dir crosshair.exe | find "crosshair.exe"
        echo.
        echo [信息] 正在启动程序...
        start crosshair.exe
        echo [信息] 程序已启动
        echo.
    ) else (
        echo.
        echo ====================================
        echo    编译失败：未生成 crosshair.exe
        echo ====================================
        echo.
    )
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


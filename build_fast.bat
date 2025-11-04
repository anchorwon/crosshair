@echo off
REM 快速开发编译脚本 - 优化编译速度
REM 移除静态链接和优化，编译速度提升 3-5 倍

chcp 65001 >nul
echo 正在快速编译...

REM 关闭正在运行的 crosshair.exe
echo 检查并关闭正在运行的程序...
taskkill /F /IM crosshair.exe >nul 2>nul
if %errorlevel% equ 0 (
    echo 已关闭正在运行的程序
    timeout /t 1 /nobreak >nul
) else (
    echo 未发现运行中的程序
)
echo.

REM 编译资源文件
echo 编译资源文件...
C:\Users\Administrator\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\windres.exe crosshair.rc -o crosshair_res.o
if %errorlevel% equ 0 (
    set RESOURCE_FILE=crosshair_res.o
) else (
    set RESOURCE_FILE=
)
echo.

C:\Users\Administrator\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe ^
-std=c++17 ^
-O0 ^
-mwindows ^
crosshair.cpp %RESOURCE_FILE% ^
-lgdiplus -lshell32 -luser32 -lgdi32 -lcomctl32 -ldwmapi ^
-o crosshair.exe

REM 清理资源对象文件
if exist crosshair_res.o del crosshair_res.o

if %errorlevel% equ 0 (
    echo.
    echo ✅ 编译成功！
    echo.
    echo 正在启动程序...
    start crosshair.exe
) else (
    echo.
    echo ❌ 编译失败！
    pause
)


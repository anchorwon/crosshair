@echo off
REM 快速开发编译脚本 - 优化编译速度
REM 移除静态链接和优化，编译速度提升 3-5 倍

echo 正在快速编译...

C:\Users\Administrator\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe ^
-std=c++17 ^
-O0 ^
-mwindows ^
crosshair.cpp ^
-lgdiplus -lshell32 -luser32 -lgdi32 -lcomctl32 -ldwmapi ^
-o crosshair.exe

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


@echo off
chcp 65001 >nul
echo ====================================
echo    屏幕准心软件 - C++ 版本编译脚本
echo ====================================
echo.

REM 检查编译器
where cl >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [错误] 未找到 MSVC 编译器！
    echo.
    echo 请先安装 Visual Studio 或 Visual Studio Build Tools
    echo 然后运行 "Developer Command Prompt for VS"
    echo 或运行 vcvarsall.bat 设置环境变量
    echo.
    pause
    exit /b 1
)

echo [信息] 找到 MSVC 编译器
echo [信息] 开始编译...
echo.

REM 编译命令
cl /EHsc /O2 /std:c++17 ^
   /D UNICODE /D _UNICODE ^
   crosshair.cpp ^
   /link ^
   gdiplus.lib ^
   shell32.lib ^
   user32.lib ^
   gdi32.lib ^
   comctl32.lib ^
   /SUBSYSTEM:WINDOWS ^
   /OUT:crosshair.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ====================================
    echo    编译成功！
    echo ====================================
    echo.
    echo 生成文件: crosshair.exe
    echo 运行方式: 双击 crosshair.exe 或运行 启动程序.bat
    echo.
    
    REM 清理中间文件
    if exist crosshair.obj del crosshair.obj
    if exist vc*.pdb del vc*.pdb
    
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


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
rc /fo crosshair.res crosshair.rc
if %ERRORLEVEL% NEQ 0 (
    echo [警告] 资源文件编译失败，将使用默认图标
    set RESOURCE_FILE=
) else (
    echo [信息] 资源文件编译成功
    set RESOURCE_FILE=crosshair.res
)
echo.

REM 编译命令
cl /EHsc /O2 /std:c++17 ^
   /D UNICODE /D _UNICODE ^
   crosshair.cpp ^
   /link ^
   %RESOURCE_FILE% ^
   gdiplus.lib ^
   shell32.lib ^
   user32.lib ^
   gdi32.lib ^
   comctl32.lib ^
   /SUBSYSTEM:WINDOWS ^
   /OUT:crosshair.exe

REM 清理资源文件
if exist crosshair.res del crosshair.res

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ====================================
    echo    编译成功！
    echo ====================================
    echo.
    echo 生成文件: crosshair.exe
    echo.
    echo [信息] 正在启动程序...
    start crosshair.exe
    echo [信息] 程序已启动
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


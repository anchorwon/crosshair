@echo off
chcp 65001 >nul
echo ====================================
echo      屏幕准心测试工具
echo ====================================
echo.

echo [1/4] 检查程序是否运行...
tasklist | findstr /i "crosshair.exe" >nul
if %ERRORLEVEL% EQU 0 (
    echo ✓ 程序正在运行
) else (
    echo ✗ 程序未运行，正在启动...
    start crosshair.exe
    timeout /t 2 /nobreak >nul
)
echo.

echo [2/4] 检查资源文件夹...
if exist resource (
    echo ✓ resource 文件夹存在
    dir /b resource\*.png 2>nul | find /c /v "" > temp.txt
    set /p count=<temp.txt
    del temp.txt
    echo   找到图片数量: %count% 个
) else (
    echo ✗ resource 文件夹不存在！
)
echo.

echo [3/4] 测试说明：
echo.
echo   现在请执行以下操作：
echo.
echo   ① 按 Ctrl+Shift+C（同时按住三个键）
echo   ② 查看屏幕中央是否出现准心
echo   ③ 如果看到准心，测试成功！
echo   ④ 如果没看到，检查系统托盘图标
echo.

echo [4/4] 系统托盘位置：
echo.
echo   请在屏幕右下角查找托盘图标
echo   如果没看到，点击向上箭头 ↑ 展开
echo.
echo   右键图标可以：
echo   - 显示/隐藏准心
echo   - 选择准心样式
echo   - 退出程序
echo.

pause


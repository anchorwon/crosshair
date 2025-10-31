@echo off
chcp 65001 >nul
echo ====================================
echo    屏幕准心软件 - 打包部署脚本
echo ====================================
echo.

:: 设置打包目录
set PACKAGE_DIR=crosshair_release
set TIMESTAMP=%date:~0,4%%date:~5,2%%date:~8,2%_%time:~0,2%%time:~3,2%%time:~6,2%
set TIMESTAMP=%TIMESTAMP: =0%

echo [1/5] 创建打包目录...
if exist "%PACKAGE_DIR%" (
    echo 删除旧的打包目录...
    rmdir /s /q "%PACKAGE_DIR%"
)
mkdir "%PACKAGE_DIR%"

echo.
echo [2/5] 复制可执行文件...
if exist "crosshair.exe" (
    copy /y "crosshair.exe" "%PACKAGE_DIR%\crosshair.exe" >nul
    echo ✓ crosshair.exe 已复制
) else (
    echo ✗ 错误: crosshair.exe 不存在！请先编译程序。
    pause
    exit /b 1
)

echo.
echo [3/5] 复制背景图片...
if exist "bg.png" (
    copy /y "bg.png" "%PACKAGE_DIR%\bg.png" >nul
    echo ✓ bg.png 已复制
) else (
    echo ⚠ 警告: bg.png 不存在，将使用默认渐变背景
)

echo.
echo [4/5] 复制 resource 文件夹...
if exist "resource\" (
    xcopy /e /i /y "resource" "%PACKAGE_DIR%\resource" >nul
    echo ✓ resource 文件夹已复制
) else (
    echo ✗ 错误: resource 文件夹不存在！
    pause
    exit /b 1
)

echo.
echo [5/5] 复制可选文件...
if exist "logo.png" (
    copy /y "logo.png" "%PACKAGE_DIR%\logo.png" >nul
    echo ✓ logo.png 已复制
)

if exist "使用说明.md" (
    copy /y "使用说明.md" "%PACKAGE_DIR%\使用说明.md" >nul
    echo ✓ 使用说明.md 已复制
)

if exist "README.md" (
    copy /y "README.md" "%PACKAGE_DIR%\README.md" >nul
    echo ✓ README.md 已复制
)

echo.
echo ====================================
echo    打包完成！
echo ====================================
echo.
echo 打包目录: %PACKAGE_DIR%
echo.
dir /b "%PACKAGE_DIR%"
echo.
echo 您可以将 %PACKAGE_DIR% 文件夹复制到其他电脑使用
echo 或者压缩为 zip 文件分发
echo.

:: 询问是否创建 zip 压缩包
echo 是否创建 ZIP 压缩包？(Y/N)
choice /c YN /n /m "请选择: "
if %errorlevel%==1 (
    echo.
    echo 正在创建 ZIP 压缩包...
    powershell -command "Compress-Archive -Path '%PACKAGE_DIR%\*' -DestinationPath 'crosshair_release_%TIMESTAMP%.zip' -Force"
    if exist "crosshair_release_%TIMESTAMP%.zip" (
        echo ✓ 压缩包已创建: crosshair_release_%TIMESTAMP%.zip
    ) else (
        echo ✗ 压缩失败
    )
)

echo.
echo 按任意键退出...
pause >nul


# 屏幕准心软件 - 打包脚本
$ErrorActionPreference = "SilentlyContinue"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   屏幕准心软件 - 打包部署" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$PACKAGE_DIR = "crosshair_release"

# 删除旧的打包目录
if (Test-Path $PACKAGE_DIR) {
    Write-Host "[1/6] 删除旧的打包目录..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $PACKAGE_DIR
}

# 创建打包目录
Write-Host "[2/6] 创建打包目录..." -ForegroundColor Green
New-Item -ItemType Directory -Path $PACKAGE_DIR | Out-Null

# 复制可执行文件
Write-Host "[3/6] 复制可执行文件..." -ForegroundColor Green
if (Test-Path "crosshair.exe") {
    Copy-Item "crosshair.exe" "$PACKAGE_DIR\" -Force
    Write-Host "  ✓ crosshair.exe" -ForegroundColor Green
} else {
    Write-Host "  ✗ 错误: crosshair.exe 不存在！" -ForegroundColor Red
    exit 1
}

# 复制背景图片
Write-Host "[4/6] 复制背景图片..." -ForegroundColor Green
if (Test-Path "bg.png") {
    Copy-Item "bg.png" "$PACKAGE_DIR\" -Force
    Write-Host "  ✓ bg.png" -ForegroundColor Green
} else {
    Write-Host "  ⚠ bg.png 不存在，将使用默认渐变背景" -ForegroundColor Yellow
}

# 复制 resource 文件夹
Write-Host "[5/6] 复制 resource 文件夹..." -ForegroundColor Green
if (Test-Path "resource") {
    Copy-Item "resource" "$PACKAGE_DIR\resource" -Recurse -Force
    $fileCount = (Get-ChildItem "$PACKAGE_DIR\resource" -File).Count
    Write-Host "  ✓ resource 文件夹 ($fileCount 个文件)" -ForegroundColor Green
} else {
    Write-Host "  ✗ 错误: resource 文件夹不存在！" -ForegroundColor Red
    exit 1
}

# 复制可选文件
Write-Host "[6/6] 复制可选文件..." -ForegroundColor Green
if (Test-Path "logo.png") {
    Copy-Item "logo.png" "$PACKAGE_DIR\" -Force
    Write-Host "  ✓ logo.png" -ForegroundColor Green
}
if (Test-Path "使用说明.md") {
    Copy-Item "使用说明.md" "$PACKAGE_DIR\" -Force
    Write-Host "  ✓ 使用说明.md" -ForegroundColor Green
}
if (Test-Path "README.md") {
    Copy-Item "README.md" "$PACKAGE_DIR\" -Force
    Write-Host "  ✓ README.md" -ForegroundColor Green
}
if (Test-Path "部署说明.txt") {
    Copy-Item "部署说明.txt" "$PACKAGE_DIR\" -Force
    Write-Host "  ✓ 部署说明.txt" -ForegroundColor Green
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   打包完成！" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "打包目录内容:" -ForegroundColor Yellow
Get-ChildItem $PACKAGE_DIR | Format-Table -Property Mode, Length, Name -AutoSize

# 计算总大小
$totalSize = (Get-ChildItem $PACKAGE_DIR -Recurse -File | Measure-Object -Property Length -Sum).Sum / 1MB
Write-Host "总大小: $([math]::Round($totalSize, 2)) MB" -ForegroundColor Cyan
Write-Host ""

# 询问是否创建 ZIP
Write-Host "是否创建 ZIP 压缩包？(Y/N)" -ForegroundColor Yellow
$response = Read-Host "请选择"
if ($response -eq 'Y' -or $response -eq 'y') {
    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $zipName = "crosshair_release_$timestamp.zip"
    Write-Host ""
    Write-Host "正在创建 ZIP 压缩包..." -ForegroundColor Green
    Compress-Archive -Path "$PACKAGE_DIR\*" -DestinationPath $zipName -Force
    if (Test-Path $zipName) {
        $zipSize = (Get-Item $zipName).Length / 1MB
        Write-Host "✓ 压缩包已创建: $zipName ($([math]::Round($zipSize, 2)) MB)" -ForegroundColor Green
    }
}

Write-Host ""
Write-Host "完成！" -ForegroundColor Green


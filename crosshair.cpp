// crosshair.cpp - 屏幕准心软件
// 使用 Windows API + GDI+ 开发

#define UNICODE
#define _UNICODE
#define NOMINMAX

#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <commdlg.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cmath>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

// 图标资源ID（与crosshair.rc中的定义一致）
#define IDI_MAINICON 1

using namespace Gdiplus;
namespace fs = std::filesystem;

// 全局变量
HINSTANCE g_hInst = nullptr;
HWND g_hwndCrosshair = nullptr;
NOTIFYICONDATA g_nid = {0};
Image* g_pCurrentImage = nullptr;
bool g_bVisible = true;  // 默认显示准心

// 配置文件路径
std::wstring g_configPath;
// Resource目录路径
std::wstring g_resourceDir;

// 准心图片映射
struct CrosshairInfo {
    std::wstring name;
    std::wstring path;
    std::wstring group;
};

std::vector<CrosshairInfo> g_crosshairs;
int g_currentIndex = 0;

// 准心缩放
float g_crosshairScale = 1.0f;  // 默认缩放比例 1.0
const float SCALE_MIN = 0.5f;   // 最小50%
const float SCALE_MAX = 2.0f;   // 最大200%
const float SCALE_STEP = 0.1f;  // 每次调整10%
bool g_isDraggingScaleSlider = false;  // 是否正在拖拽滑块

// 颜色自定义
struct CrosshairColor {
    std::wstring name;
    Color color;
};

std::vector<CrosshairColor> g_colors = {
    {L"红色", Color(255, 255, 0, 0)},
    {L"蓝色", Color(255, 0, 150, 255)},
    {L"绿色", Color(255, 0, 255, 0)},
    {L"黄色", Color(255, 255, 255, 0)},
    {L"紫色", Color(255, 200, 0, 255)},
    {L"青色", Color(255, 0, 255, 255)},
    {L"橙色", Color(255, 255, 165, 0)},
    {L"粉色", Color(255, 255, 105, 180)}
};
int g_currentColorIndex = 0;  // 默认选中红色

// 预览窗口相关
HWND g_hwndPreview = nullptr;
bool g_bPreviewWindowVisible = false;

// Switch动画相关（已移除动画，直接使用状态）
float g_switchAnimPos = 0.0f;  // 0.0 = 关闭位置, 1.0 = 开启位置（不再使用动画）
bool g_switchAnimating = false;  // 不再使用
const UINT TIMER_SWITCH_ANIM = 100;  // 不再使用

// 重置图标旋转动画
float g_resetIconRotation = 0.0f;  // 当前旋转角度
bool g_resetIconAnimating = false;  // 是否正在旋转动画
const UINT TIMER_RESET_ANIM = 101;
const UINT TIMER_KEEP_TOPMOST = 102;  // 保持置顶定时器
const UINT TIMER_GIF_ANIM = 103;  // GIF动画定时器
const float RESET_ROTATION_SPEED = 30.0f;  // 每帧旋转角度

// GIF动画相关
int g_gifCurrentFrame = 0;  // 当前GIF帧索引
int g_gifFrameCount = 0;  // GIF总帧数
bool g_isGifAnimation = false;  // 是否是GIF动画
GUID g_gifFrameDimension = FrameDimensionTime;  // GIF帧维度
const float SWITCH_ANIM_SPEED = 0.2f;  // 动画速度（提高速度）
const float SWITCH_ANIM_DAMPING = 0.85f;  // 阻尼系数（平滑过渡）

// Tab切换相关
std::vector<std::wstring> g_tabGroups;  // 所有的组名
int g_currentTabIndex = 0;  // 当前选中的Tab索引
float g_tabSliderPos = 0.0f;  // Tab滑块位置（动画用）
int g_tabSliderTargetIndex = 0;  // 滑块目标Tab索引

// 鼠标悬停状态
int g_hoveredCrosshairIndex = -1;  // 当前鼠标悬停的准心索引（-1表示未悬停）
int g_rightClickedCrosshairIndex = -1;  // 当前右键点击的准心索引（用于菜单操作）
HWND g_hwndMenu = nullptr;  // 自定义菜单窗口
bool g_menuItemHovered = false;  // 菜单项是否悬停

// 自定义标题栏
const int TITLEBAR_HEIGHT = 40;
bool g_bCloseButtonHover = false;

// 窗口类名和托盘ID
const wchar_t* CLASS_NAME = L"CrosshairWindowClass";
const wchar_t* PREVIEW_CLASS_NAME = L"CrosshairPreviewWindowClass";
const wchar_t* MENU_CLASS_NAME = L"CrosshairMenuWindowClass";
const UINT WM_TRAYICON = WM_USER + 1;
const UINT WM_PREVIEW_SELECT = WM_USER + 2;
const UINT TRAY_ID = 1;
const UINT IDM_TOGGLE = 1001;
const UINT IDM_EXIT = 1002;
const UINT IDM_SHOW_SETTINGS = 1003;
const UINT IDM_CROSSHAIR_BASE = 2000;
const UINT IDM_COLOR_BASE = 3000;
const UINT IDM_DELETE_CROSSHAIR = 4001;

// 热键ID
const int HOTKEY_ID = 1;

// 函数声明
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK PreviewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MenuWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ShowDeleteMenu(HWND hwnd, int x, int y);
void LoadCrosshairImages();
void CreateTrayIcon(HWND hwnd);
void ShowContextMenu(HWND hwnd);
void SetCrosshair(int index);
void ToggleVisibility();
void CenterWindow(HWND hwnd);
void SaveConfig();
void LoadConfig();
std::wstring GetGroupName(const std::wstring& name);
std::wstring GetResourceDir();  // 获取resource目录的绝对路径
HICON CreateIconFromImage(Image* img);
void CreatePreviewWindow();
void ShowPreviewWindow();
void HidePreviewWindow();
void ApplyColorToCrosshair();
void DrawColoredImage(Graphics& graphics, Image* img, int x, int y, int width, int height, const Color& tintColor);
float EaseInOutCubic(float t);  // 缓动函数
void AdjustCrosshairScale(float delta);  // 调整准心缩放
bool IsCustomCrosshair(int index);  // 判断是否是自定义准心
void DeleteCustomCrosshair(int index);  // 删除自定义准心

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInst = hInstance;
    
    // 获取程序所在目录并设置配置文件路径
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH);
    std::wstring exeDir = exePath;
    size_t lastSlash = exeDir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        exeDir = exeDir.substr(0, lastSlash);
    }
    g_configPath = exeDir + L"\\crosshair_config.ini";
    // 直接构造绝对路径（exeDir已经是绝对路径）
    g_resourceDir = exeDir + L"\\resource";
    
    // 初始化 GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Status status = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    
    if (status != Ok) {
        MessageBox(nullptr, L"GDI+ 初始化失败！", L"错误", MB_ICONERROR);
        return 1;
    }
    
    // 加载程序图标
    HICON hAppIcon = nullptr;
    HICON hAppIconSmall = nullptr;
    
    // 优先从资源文件加载嵌入的图标（让exe文件本身显示图标）
    hAppIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    hAppIconSmall = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, 16, 16, 0);
    
    // 如果从资源加载失败，尝试从文件系统加载
    if (!hAppIcon) {
        // 优先尝试加载 logo.ico
        if (fs::exists(L"logo.ico")) {
            hAppIcon = (HICON)LoadImage(nullptr, L"logo.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
            hAppIconSmall = (HICON)LoadImage(nullptr, L"logo.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
        }
        // 如果 ico 不存在，尝试从 logo.png 创建图标
        else if (fs::exists(L"logo.png")) {
            Image* logoImg = Image::FromFile(L"logo.png");
            if (logoImg && logoImg->GetLastStatus() == Ok) {
                hAppIcon = CreateIconFromImage(logoImg);
                // 创建小图标（16x16）
                Bitmap* smallBitmap = new Bitmap(16, 16, PixelFormat32bppARGB);
                Graphics smallGraphics(smallBitmap);
                smallGraphics.SetSmoothingMode(SmoothingModeHighQuality);
                smallGraphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
                smallGraphics.DrawImage(logoImg, 0, 0, 16, 16);
                smallBitmap->GetHICON(&hAppIconSmall);
                delete smallBitmap;
                delete logoImg;
            }
        }
    }
    
    // 如果都没有，使用默认图标
    if (!hAppIcon) {
        hAppIcon = LoadIcon(nullptr, IDI_APPLICATION);
        hAppIconSmall = LoadIcon(nullptr, IDI_APPLICATION);
    }
    
    // 注册准心窗口类
    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = CLASS_NAME;
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hIcon = hAppIcon;  // 设置窗口图标
    wcex.hIconSm = hAppIconSmall;  // 设置小图标
    
    if (!RegisterClassEx(&wcex)) {
        MessageBox(nullptr, L"窗口类注册失败！", L"错误", MB_ICONERROR);
        return 1;
    }
    
    // 注册预览窗口类
    WNDCLASSEX wcexPreview = {0};
    wcexPreview.cbSize = sizeof(WNDCLASSEX);
    wcexPreview.lpfnWndProc = PreviewWndProc;
    wcexPreview.hInstance = hInstance;
    wcexPreview.lpszClassName = PREVIEW_CLASS_NAME;
    wcexPreview.hbrBackground = nullptr;  // 不使用系统背景，避免边框
    wcexPreview.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcexPreview.style = CS_HREDRAW | CS_VREDRAW;
    wcexPreview.hIcon = hAppIcon;  // 设置窗口图标
    wcexPreview.hIconSm = hAppIconSmall;  // 设置小图标
    
    if (!RegisterClassEx(&wcexPreview)) {
        MessageBox(nullptr, L"预览窗口类注册失败！", L"错误", MB_ICONERROR);
        return 1;
    }
    
    // 注册自定义菜单窗口类
    WNDCLASSEX wcexMenu = {0};
    wcexMenu.cbSize = sizeof(WNDCLASSEX);
    wcexMenu.lpfnWndProc = MenuWndProc;
    wcexMenu.hInstance = hInstance;
    wcexMenu.lpszClassName = MENU_CLASS_NAME;
    wcexMenu.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcexMenu.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcexMenu.style = CS_HREDRAW | CS_VREDRAW;
    
    if (!RegisterClassEx(&wcexMenu)) {
        MessageBox(nullptr, L"菜单窗口类注册失败！", L"错误", MB_ICONERROR);
        return 1;
    }
    
    // 创建窗口（隐藏、无边框、置顶、透明）
    g_hwndCrosshair = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        CLASS_NAME,
        L"屏幕准心",
        WS_POPUP,
        0, 0, 100, 100,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    
    if (!g_hwndCrosshair) {
        MessageBox(nullptr, L"窗口创建失败！", L"错误", MB_ICONERROR);
        return 1;
    }
    
    // 设置分层窗口属性（透明背景）
    SetLayeredWindowAttributes(g_hwndCrosshair, RGB(0, 0, 0), 0, LWA_COLORKEY);
    
    // 启动置顶保持定时器（每500ms强制置顶一次，兼容全屏游戏）
    SetTimer(g_hwndCrosshair, TIMER_KEEP_TOPMOST, 500, nullptr);
    
    // 加载准心图片
    LoadCrosshairImages();
    
    // 检查是否加载了准心
    if (g_crosshairs.empty()) {
        // 检查 resource 文件夹是否存在
        std::wstring resourceDirStr = GetResourceDir();
        fs::path resourceDir = resourceDirStr;
        if (!fs::exists(resourceDir)) {
            MessageBox(nullptr, 
                L"❌ 找不到 resource 文件夹！\n\n"
                L"程序需要以下文件结构：\n"
                L"  crosshair.exe\n"
                L"  resource\\        ← 准心图片文件夹\n"
                L"    ├── 丁字 (1).png\n"
                L"    ├── 十字 (1).png\n"
                L"    └── ...\n\n"
                L"请将 resource 文件夹复制到程序目录。",
                L"缺少 resource 文件夹", 
                MB_ICONERROR);
        } else {
            MessageBox(nullptr, 
                L"⚠️ resource 文件夹是空的！\n\n"
                L"请将准心图片（PNG/JPG）放入 resource 文件夹。",
                L"未找到准心图片", 
                MB_ICONWARNING);
        }
    } else {
        // 加载保存的配置
        LoadConfig();
        
        // 设置准心（使用配置中的索引）
        SetCrosshair(g_currentIndex);
        
        // 显示准心（根据配置中的显示状态）
        if (g_bVisible) {
            ShowWindow(g_hwndCrosshair, SW_SHOW);
            SetWindowPos(g_hwndCrosshair, HWND_TOPMOST, 0, 0, 0, 0, 
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            // 强制重绘窗口，确保准心立即显示
            InvalidateRect(g_hwndCrosshair, nullptr, TRUE);
            UpdateWindow(g_hwndCrosshair);
        }
    }
    
    // 创建系统托盘图标
    CreateTrayIcon(g_hwndCrosshair);
    
    // 注册全局热键 Ctrl+Shift+C
    RegisterHotKey(g_hwndCrosshair, HOTKEY_ID, MOD_CONTROL | MOD_SHIFT, 'C');
    
    // 显示通知
    g_nid.uFlags |= NIF_INFO;
    wcscpy_s(g_nid.szInfoTitle, L"屏幕准心");
    wcscpy_s(g_nid.szInfo, L"程序已启动\n按 Ctrl+Shift+C 显示/隐藏准心\n点击托盘图标打开设置窗口");
    g_nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIcon(NIM_MODIFY, &g_nid);
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // 清理
    UnregisterHotKey(g_hwndCrosshair, HOTKEY_ID);
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
    // 停止GIF动画定时器
    if (g_hwndCrosshair) {
        KillTimer(g_hwndCrosshair, TIMER_GIF_ANIM);
    }
    if (g_pCurrentImage) delete g_pCurrentImage;
    GdiplusShutdown(gdiplusToken);
    
    return (int)msg.wParam;
}

// 窗口过程
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            if (g_pCurrentImage) {
                Graphics graphics(hdc);
                graphics.SetSmoothingMode(SmoothingModeHighQuality);
                graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
                
                // 绘制准心图片（应用颜色和缩放）
                int scaledWidth = (int)(g_pCurrentImage->GetWidth() * g_crosshairScale);
                int scaledHeight = (int)(g_pCurrentImage->GetHeight() * g_crosshairScale);
                
                // 如果是自定义准心，不应用颜色，直接绘制原图
                if (IsCustomCrosshair(g_currentIndex)) {
                    graphics.DrawImage(g_pCurrentImage, 0, 0, scaledWidth, scaledHeight);
                } else {
                DrawColoredImage(graphics, g_pCurrentImage, 0, 0, 
                    scaledWidth, scaledHeight,
                    g_colors[g_currentColorIndex].color);
                }
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_TRAYICON: {
            if (lParam == WM_RBUTTONUP) {
                // 右键点击托盘图标，显示菜单
                ShowContextMenu(hwnd);
            } else if (lParam == WM_LBUTTONDBLCLK) {
                // 双击托盘图标，打开设置窗口
                ShowPreviewWindow();
            }
            return 0;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            
            if (wmId == IDM_TOGGLE) {
                ToggleVisibility();
            } else if (wmId == IDM_SHOW_SETTINGS) {
                ShowPreviewWindow();
            } else if (wmId == IDM_EXIT) {
                PostQuitMessage(0);
            } else if (wmId >= IDM_CROSSHAIR_BASE && wmId < IDM_CROSSHAIR_BASE + 1000) {
                // 选择准心
                int index = wmId - IDM_CROSSHAIR_BASE;
                if (index >= 0 && index < g_crosshairs.size()) {
                    SetCrosshair(index);
                }
            } else if (wmId >= IDM_COLOR_BASE && wmId < IDM_COLOR_BASE + 100) {
                // 选择颜色
                int colorIndex = wmId - IDM_COLOR_BASE;
                if (colorIndex >= 0 && colorIndex < g_colors.size()) {
                    g_currentColorIndex = colorIndex;
                    ApplyColorToCrosshair();
                    SaveConfig();  // 保存配置
                }
            }
            return 0;
        }
        
        case WM_PREVIEW_SELECT: {
            // 从预览窗口选择准心
            int index = (int)wParam;
            if (index >= 0 && index < g_crosshairs.size()) {
                SetCrosshair(index);
                // 不再自动隐藏窗口，让用户可以继续选择其他准心
                // 自动显示准心
                if (!g_bVisible) {
                    g_bVisible = true;
                    ShowWindow(g_hwndCrosshair, SW_SHOW);
                    SetWindowPos(g_hwndCrosshair, HWND_TOPMOST, 0, 0, 0, 0, 
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    
                    // 刷新预览窗口以更新开关状态（无动画）
                    if (g_hwndPreview && g_bPreviewWindowVisible) {
                        InvalidateRect(g_hwndPreview, nullptr, FALSE);
                    }
                }
                
                // 刷新预览窗口以更新选中状态
                if (g_hwndPreview && g_bPreviewWindowVisible) {
                    InvalidateRect(g_hwndPreview, nullptr, FALSE);
                    UpdateWindow(g_hwndPreview);
                }
            }
            return 0;
        }
        
        case WM_HOTKEY: {
            if (wParam == HOTKEY_ID) {
                ToggleVisibility();
            }
            return 0;
        }
        
        case WM_TIMER: {
            if (wParam == TIMER_KEEP_TOPMOST) {
                // 定期强制窗口置顶（对抗全屏游戏）
                if (g_bVisible && IsWindowVisible(hwnd)) {
                    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
                }
            } else if (wParam == TIMER_GIF_ANIM) {
                // GIF动画帧更新
                if (g_isGifAnimation && g_pCurrentImage && g_gifFrameCount > 1) {
                    // 切换到下一帧
                    g_gifCurrentFrame = (g_gifCurrentFrame + 1) % g_gifFrameCount;
                    g_pCurrentImage->SelectActiveFrame(&g_gifFrameDimension, g_gifCurrentFrame);
                    
                    // 重绘窗口以显示新帧
                    InvalidateRect(hwnd, nullptr, TRUE);
                    UpdateWindow(hwnd);
                }
            }
            return 0;
        }
        
        case WM_DESTROY:
            // 清理定时器
            KillTimer(hwnd, TIMER_KEEP_TOPMOST);
            KillTimer(hwnd, TIMER_GIF_ANIM);
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 加载准心图片
void LoadCrosshairImages() {
    std::wstring resourceDirStr = GetResourceDir();
    fs::path resourceDir = resourceDirStr;
    
    if (!fs::exists(resourceDir) || !fs::is_directory(resourceDir)) {
        MessageBox(nullptr, L"resource 文件夹不存在！", L"警告", MB_ICONWARNING);
        return;
    }
    
    std::vector<std::wstring> extensions = {L".png", L".jpg", L".jpeg", L".bmp", L".gif"};
    
    for (const auto& entry : fs::directory_iterator(resourceDir)) {
        if (!entry.is_regular_file()) continue;
        
        std::wstring ext = entry.path().extension().wstring();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
            // 排除非准心图片（bg.png 用作背景，logo.png 用作图标）
            std::wstring filename = entry.path().filename().wstring();
            if (filename == L"bg.png" || filename == L"logo.png") {
                continue;
            }
            
            CrosshairInfo info;
            info.name = entry.path().stem().wstring();
            // 构造绝对路径（resourceDir已经是绝对路径）
            std::wstring filePath = resourceDirStr + L"\\" + entry.path().filename().wstring();
            info.path = filePath;
            info.group = GetGroupName(info.name);
            g_crosshairs.push_back(info);
        }
    }
    
    // 按分组和名称排序
    std::sort(g_crosshairs.begin(), g_crosshairs.end(), 
        [](const CrosshairInfo& a, const CrosshairInfo& b) {
            if (a.group != b.group) return a.group < b.group;
            return a.name < b.name;
        });
    
    // 提取所有唯一的分组名称
    g_tabGroups.clear();
    
    // 使用 set 去重，避免重复添加
    std::vector<std::wstring> tempGroups;
    for (const auto& crosshair : g_crosshairs) {
        if (crosshair.group != L"自定义") {
            // 检查是否已存在
            if (std::find(tempGroups.begin(), tempGroups.end(), crosshair.group) == tempGroups.end()) {
                tempGroups.push_back(crosshair.group);
        }
    }
    }
    
    // 按字典序排序（可选，保持一致的显示顺序）
    std::sort(tempGroups.begin(), tempGroups.end());
    
    // 添加到 g_tabGroups
    for (const auto& group : tempGroups) {
        g_tabGroups.push_back(group);
    }
    
    // 始终将"自定义"分组放在最右侧
    g_tabGroups.push_back(L"自定义");
}

// 获取准心分组名称
std::wstring GetGroupName(const std::wstring& name) {
    if (name.find(L"丁字") != std::wstring::npos) return L"丁字";
    if (name.find(L"十字") != std::wstring::npos) return L"十字";
    if (name.find(L"圆点") != std::wstring::npos) return L"圆点";
    if (name.find(L"小圆") != std::wstring::npos) return L"小圆";
    if (name.find(L"指示圈") != std::wstring::npos) return L"指示圈";
    if (name.find(L"战术") != std::wstring::npos || name.find(L"M145") != std::wstring::npos) 
        return L"战术";
    return L"自定义";
}

// 获取resource目录的绝对路径（使用exe目录）
std::wstring GetResourceDir() {
    // 优先使用全局变量，确保一致性
    if (!g_resourceDir.empty()) {
        return g_resourceDir;
    }

    // 使用GetModuleFileName获取exe完整路径
    wchar_t exePath[MAX_PATH];
    DWORD result = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (result == 0 || result == MAX_PATH) {
        // 获取失败，使用应用数据目录作为后备
        char appDataPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
            std::string pathStr = std::string(appDataPath) + "\\Crosshair\\resource";
            return std::wstring(pathStr.begin(), pathStr.end());
        }
        return L"./resource"; // 最后的回退方案
    }

    // 将exePath复制到一个可以修改的缓冲区
    wchar_t exeDir[MAX_PATH];
    wcscpy_s(exeDir, exePath);

    // 使用PathRemoveFileSpec移除文件名，只保留目录
    if (PathRemoveFileSpecW(exeDir)) {
        // 拼接resource路径
        std::wstring resourcePath = std::wstring(exeDir) + L"\\resource";
        return resourcePath;
    }

    // 如果PathRemoveFileSpec失败，手动查找最后一个斜杠
    std::wstring exePathStr = exePath;
    size_t lastSlash = exePathStr.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        exePathStr = exePathStr.substr(0, lastSlash);
        return exePathStr + L"\\resource";
    }

    // 最后的回退方案
    return L"./resource";
}

// 从 GDI+ Image 创建 HICON
HICON CreateIconFromImage(Image* img) {
    if (!img || img->GetLastStatus() != Ok) return nullptr;
    
    // 转换为 Bitmap
    Bitmap* bitmap = static_cast<Bitmap*>(img);
    if (!bitmap) {
        // 如果不是 Bitmap，创建一个
        bitmap = new Bitmap(img->GetWidth(), img->GetHeight(), PixelFormat32bppARGB);
        Graphics graphics(bitmap);
        graphics.DrawImage(img, 0, 0, img->GetWidth(), img->GetHeight());
    }
    
    // 获取 HICON
    HICON hIcon;
    Status status = bitmap->GetHICON(&hIcon);
    
    return (status == Ok) ? hIcon : nullptr;
}

// 创建系统托盘图标
void CreateTrayIcon(HWND hwnd) {
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = hwnd;
    g_nid.uID = TRAY_ID;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    
    // 尝试加载 logo.png 作为图标
    HICON hIcon = nullptr;
    if (fs::exists(L"logo.png")) {
        Image* logoImg = Image::FromFile(L"logo.png");
        if (logoImg && logoImg->GetLastStatus() == Ok) {
            hIcon = CreateIconFromImage(logoImg);
            delete logoImg;
        }
    }
    
    // 如果加载失败，使用默认图标
    if (!hIcon) {
        hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }
    
    g_nid.hIcon = hIcon;
    wcscpy_s(g_nid.szTip, L"屏幕准心 - 双击打开设置，右键打开菜单");
    
    Shell_NotifyIcon(NIM_ADD, &g_nid);
}

// 显示托盘右键菜单
void ShowContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    
    HMENU hMenu = CreatePopupMenu();
    
    // 添加三个菜单项
    AppendMenu(hMenu, MF_STRING, IDM_TOGGLE, g_bVisible ? L"隐藏准心" : L"显示准心");
    AppendMenu(hMenu, MF_STRING, IDM_SHOW_SETTINGS, L"应用程序");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"退出");
    
    // 显示菜单
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
    PostMessage(hwnd, WM_NULL, 0, 0);
    
    DestroyMenu(hMenu);
}

// 设置准心
void SetCrosshair(int index) {
    if (index < 0 || index >= g_crosshairs.size()) return;
    
    // 删除旧图片
    if (g_pCurrentImage) {
        delete g_pCurrentImage;
        g_pCurrentImage = nullptr;
    }
    
    // 加载新图片
    g_pCurrentImage = Image::FromFile(g_crosshairs[index].path.c_str());
    
    if (!g_pCurrentImage || g_pCurrentImage->GetLastStatus() != Ok) {
        MessageBox(nullptr, L"加载图片失败！", L"错误", MB_ICONERROR);
        return;
    }
    
    g_currentIndex = index;
    
    // 停止之前的GIF动画定时器
    KillTimer(g_hwndCrosshair, TIMER_GIF_ANIM);
    g_isGifAnimation = false;
    g_gifCurrentFrame = 0;
    g_gifFrameCount = 0;
    
    // 检测是否是GIF文件并获取帧信息
    std::wstring ext = g_crosshairs[index].path;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext.find(L".gif") != std::wstring::npos) {
        // 获取GIF帧数
        UINT frameCount = g_pCurrentImage->GetFrameDimensionsCount();
        if (frameCount > 0) {
            GUID* pDimensionIDs = new GUID[frameCount];
            g_pCurrentImage->GetFrameDimensionsList(pDimensionIDs, frameCount);
            
            // 查找时间维度
            for (UINT i = 0; i < frameCount; i++) {
                if (pDimensionIDs[i] == FrameDimensionTime) {
                    g_gifFrameCount = g_pCurrentImage->GetFrameCount(&pDimensionIDs[i]);
                    if (g_gifFrameCount > 1) {
                        g_isGifAnimation = true;
                        g_gifFrameDimension = FrameDimensionTime;
                        g_gifCurrentFrame = 0;
                        // 设置为第一帧
                        g_pCurrentImage->SelectActiveFrame(&g_gifFrameDimension, 0);
                        // 启动GIF动画定时器（每50ms更新一次，约20fps）
                        SetTimer(g_hwndCrosshair, TIMER_GIF_ANIM, 50, nullptr);
                    }
                    break;
                }
            }
            delete[] pDimensionIDs;
        }
    }
    
    // 调整窗口大小（应用缩放）
    int width = (int)(g_pCurrentImage->GetWidth() * g_crosshairScale);
    int height = (int)(g_pCurrentImage->GetHeight() * g_crosshairScale);
    SetWindowPos(g_hwndCrosshair, HWND_TOPMOST, 0, 0, width, height, 
        SWP_NOMOVE | SWP_NOACTIVATE);
    
    // 居中窗口
    CenterWindow(g_hwndCrosshair);
    
    // 如果不是自定义准心，应用当前颜色到准心
    if (!IsCustomCrosshair(index)) {
    ApplyColorToCrosshair();
    } else {
        // 自定义准心不应用颜色，直接刷新显示
        InvalidateRect(g_hwndCrosshair, nullptr, TRUE);
        UpdateWindow(g_hwndCrosshair);
    }
    
    // 重绘窗口
    InvalidateRect(g_hwndCrosshair, nullptr, TRUE);
    UpdateWindow(g_hwndCrosshair);  // 立即更新窗口
    
    // 保存配置
    SaveConfig();
    
    // 不再显示通知，避免打扰用户
}

// 切换显示/隐藏
void ToggleVisibility() {
    g_bVisible = !g_bVisible;
    ShowWindow(g_hwndCrosshair, g_bVisible ? SW_SHOW : SW_HIDE);
    
    if (g_bVisible) {
        SetWindowPos(g_hwndCrosshair, HWND_TOPMOST, 0, 0, 0, 0, 
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        // 强制重绘窗口，确保准心正确显示
        InvalidateRect(g_hwndCrosshair, nullptr, TRUE);
        UpdateWindow(g_hwndCrosshair);
    }
    
    // 保存配置
    SaveConfig();
    
    // 如果预览窗口打开，刷新显示（无动画）
    if (g_hwndPreview && g_bPreviewWindowVisible) {
        InvalidateRect(g_hwndPreview, nullptr, FALSE);
    }
}

// 窗口居中
void CenterWindow(HWND hwnd) {
    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);
    
    int width = rcWindow.right - rcWindow.left;
    int height = rcWindow.bottom - rcWindow.top;
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;
    
    SetWindowPos(hwnd, HWND_TOPMOST, x, y, 0, 0, 
        SWP_NOSIZE | SWP_NOACTIVATE);
}

// 预览窗口过程
LRESULT CALLBACK PreviewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    const int ITEM_WIDTH = 120;
    const int ITEM_HEIGHT = 140;
    const int ITEMS_PER_ROW = 4;
    const int MARGIN = 22;  // 从15增加到22，增加整体边距
    
    switch (msg) {
        case WM_CREATE: {
            // Switch动画已移除，不再需要初始化动画位置
            
            // 不再使用滚动条，改用Tab标签页切换
            return 0;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // 获取客户区大小
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            // 创建双缓冲 - 避免闪烁
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
            
            // 在内存DC中绘制界面 - 深色现代主题（参考NVIDIA GeForce）
            Graphics graphics(hdcMem);
            graphics.SetSmoothingMode(SmoothingModeHighQuality);
            graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
            
            // 整体背景 - 使用背景图片
            Image* bgImage = Image::FromFile(L"bg.png");
            if (bgImage && bgImage->GetLastStatus() == Ok) {
                // 绘制背景图片，拉伸填充整个窗口
                graphics.DrawImage(bgImage, 0, 0, rc.right, rc.bottom);
                delete bgImage;
            } else {
                // 如果图片加载失败，使用径向渐变作为后备
                Color centerColor(255, 37, 15, 43);   // #250f2b 深紫色（中心）
                Color edgeColor(255, 7, 1, 18);       // #070112 深黑色（边缘）
                
                GraphicsPath path;
                path.AddRectangle(Rect(0, 0, rc.right, rc.bottom));
                
                PathGradientBrush radialBrush(&path);
                radialBrush.SetCenterColor(centerColor);
                radialBrush.SetCenterPoint(PointF(rc.right / 2.0f, rc.bottom / 2.0f));
                
                int colorsCount = 1;
                radialBrush.SetSurroundColors(&edgeColor, &colorsCount);
                
                graphics.FillRectangle(&radialBrush, 0, 0, rc.right, rc.bottom);
            }
            
            // 自定义标题栏 - 半透明黑色，让径向渐变透出来
            SolidBrush titleBarBrush(Color(60, 0, 0, 0));  // 半透明黑色
            graphics.FillRectangle(&titleBarBrush, 0, 0, rc.right, TITLEBAR_HEIGHT);
            
            // 绘制关闭按钮（纯图标，无背景）- 右边距较小
            const int closeButtonSize = 40;
            const int closeButtonMargin = 8;  // 关闭按钮专用较小边距
            const int closeButtonX = rc.right - closeButtonMargin - closeButtonSize;
            const int closeButtonY = 0;
            
            // 绘制 X 图标（简洁线条）- 悬停时变白，无背景
            // 默认颜色：80%不透明的灰色 (类似 color-mix oklab)
            // 悬停颜色：纯白色
            Color closeIconColor = g_bCloseButtonHover 
                ? Color(255, 255, 255, 255)      // 悬停：纯白色
                : Color(204, 163, 163, 163);     // 默认：80%不透明灰色 (204/255 ≈ 0.8)
            Pen closePen(closeIconColor, 1.5f);
            closePen.SetLineCap(LineCapRound, LineCapRound, DashCapFlat);
            int iconMargin = 14;
            graphics.DrawLine(&closePen, 
                closeButtonX + iconMargin, closeButtonY + iconMargin,
                closeButtonX + closeButtonSize - iconMargin, closeButtonY + closeButtonSize - iconMargin);
            graphics.DrawLine(&closePen, 
                closeButtonX + closeButtonSize - iconMargin, closeButtonY + iconMargin,
                closeButtonX + iconMargin, closeButtonY + closeButtonSize - iconMargin);
            
            // 顶部工具栏背景 - 半透明黑色叠加，保留渐变效果
            SolidBrush toolbarBgBrush(Color(40, 0, 0, 0));  // 半透明黑色
            graphics.FillRectangle(&toolbarBgBrush, 0, TITLEBAR_HEIGHT, rc.right, 130 - TITLEBAR_HEIGHT);
            
            // 第一行：准心标题（左侧） + Switch开关（右侧）
            Font titleFont(L"Microsoft YaHei UI", 20, FontStyleBold);
            SolidBrush titleBrush(Color(255, 255, 255, 255));  // 白色文字
            graphics.DrawString(L"准心配置", -1, &titleFont, PointF(MARGIN, TITLEBAR_HEIGHT + 10), &titleBrush);
            
            // 绘制Switch滑块（开关按钮，带动画）- 放在右侧（调小尺寸）
            const int switchWidth = 50;   // 从60改为50
            const int switchHeight = 26;  // 从30改为26
            const int switchX = rc.right - MARGIN - switchWidth;  // 右对齐
            const int switchY = TITLEBAR_HEIGHT + 10;  // 微调垂直位置
            
            // 滑块轨道背景
            GraphicsPath switchPath;
            switchPath.AddArc(switchX, switchY, switchHeight, switchHeight, 90.0f, 180.0f);
            switchPath.AddArc(switchX + switchWidth - switchHeight, switchY, switchHeight, switchHeight, 270.0f, 180.0f);
            switchPath.CloseFigure();
            
            // 根据显示状态直接设置轨道颜色（无动画）
            // 关闭时：深黑色 (30, 30, 30) - 与Tab栏风格一致
            // 开启时：绿色 (76, 175, 80)
            Color trackColor = g_bVisible 
                ? Color(255, 76, 175, 80)   // 开启：绿色
                : Color(255, 30, 30, 30);  // 关闭：深黑色
            SolidBrush trackBrush(trackColor);
            graphics.FillPath(&trackBrush, &switchPath);
            
            // 绘制滑块圆形按钮（直接根据状态定位，无动画）
            int thumbMinX = switchX + 3;
            int thumbMaxX = switchX + switchWidth - switchHeight + 3;
            int thumbX = g_bVisible ? thumbMaxX : thumbMinX;
            int thumbY = switchY + 3;
            int thumbSize = switchHeight - 6;
            
            SolidBrush thumbBrush(Color(255, 255, 255, 255));  // 白色圆形按钮
            graphics.FillEllipse(&thumbBrush, thumbX, thumbY, thumbSize, thumbSize);
            
            
            // 第三行：准心颜色标题 + 颜色选择（一行显示）
            Font colorTitleFont(L"Microsoft YaHei UI", 11, FontStyleBold);
            SolidBrush colorTitleBrush(Color(255, 255, 255, 255));
            graphics.DrawString(L"颜色", -1, &colorTitleFont, PointF(MARGIN, TITLEBAR_HEIGHT + 80), &colorTitleBrush);
            
            // 绘制圆形颜色选择 - 一行显示所有颜色
            const int COLOR_SIZE = 26;
            const int COLOR_SPACING = 12;
            int colorStartX = MARGIN + 60;  // 在"颜色"标题右侧
            int colorStartY = TITLEBAR_HEIGHT + 77;
            
            for (size_t i = 0; i < g_colors.size(); ++i) {
                int colorX = colorStartX + i * (COLOR_SIZE + COLOR_SPACING);
                int colorY = colorStartY;
                
                // 填充颜色（圆形）
                SolidBrush colorBrush(g_colors[i].color);
                graphics.FillEllipse(&colorBrush, colorX, colorY, COLOR_SIZE, COLOR_SIZE);
                
                // 绘制边框和选中状态
                if (i == g_currentColorIndex) {
                    // 选中态：70%透明灰色描边（较细）
                    Pen glowPen(Color(178, 180, 180, 180), 2.0f);  // 70%透明亮灰色
                    graphics.DrawEllipse(&glowPen, colorX - 2, colorY - 2, COLOR_SIZE + 4, COLOR_SIZE + 4);
                    Pen borderPen(Color(178, 200, 200, 200), 1.5f);  // 70%透明浅灰色
                    graphics.DrawEllipse(&borderPen, colorX, colorY, COLOR_SIZE, COLOR_SIZE);
                } else {
                    // 未选中态：深灰色边框
                    Pen borderPen(Color(255, 100, 100, 100), 1.5f);
                    graphics.DrawEllipse(&borderPen, colorX, colorY, COLOR_SIZE, COLOR_SIZE);
                }
            }
            
            // 第四行：准心大小滑块控制
            Font sizeTitleFont(L"Microsoft YaHei UI", 11, FontStyleBold);
            SolidBrush sizeTitleBrush(Color(255, 255, 255, 255));
            graphics.DrawString(L"大小", -1, &sizeTitleFont, PointF(MARGIN, TITLEBAR_HEIGHT + 120), &sizeTitleBrush);
            
            // 绘制滑块轨道
            const int scaleSliderTrackWidth = 200;
            const int scaleSliderTrackHeight = 4;
            const int scaleSliderTrackX = MARGIN + 60;  // 与颜色圆圈左对齐
            const int scaleSliderTrackY = TITLEBAR_HEIGHT + 128;
            
            // 绘制缩放百分比（放在进度条右侧）
            wchar_t scaleText[32];
            swprintf(scaleText, 32, L"%d%%", (int)(g_crosshairScale * 100));
            Font scaleFont(L"Microsoft YaHei UI", 10);
            SolidBrush scaleBrush(Color(255, 180, 180, 180));
            int scaleTextX = scaleSliderTrackX + scaleSliderTrackWidth + 10;
            int scaleTextY = TITLEBAR_HEIGHT + 122;
            graphics.DrawString(scaleText, -1, &scaleFont, PointF(scaleTextX, scaleTextY), &scaleBrush);
            
            // 绘制重置图标（刷新图标）
            const int resetIconSize = 12;  // 缩小图标
            const int resetIconX = scaleTextX + 48;  // 百分比文字后方
            const int resetIconY = scaleTextY + 4;
            
            // 保存当前图形状态
            GraphicsState state = graphics.Save();
            
            // 应用旋转变换（围绕图标中心旋转）
            int centerX = resetIconX + resetIconSize / 2;
            int centerY = resetIconY + resetIconSize / 2;
            graphics.TranslateTransform(centerX, centerY);
            graphics.RotateTransform(g_resetIconRotation);
            graphics.TranslateTransform(-centerX, -centerY);
            
            // 绘制圆形刷新箭头
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
            Pen resetPen(Color(180, 160, 160, 160), 1.5f);
            resetPen.SetStartCap(LineCapRound);
            resetPen.SetEndCap(LineCapArrowAnchor);
            
            // 绘制弧形箭头（从270度开始，顺时针270度）
            graphics.DrawArc(&resetPen, resetIconX, resetIconY, resetIconSize, resetIconSize, 45, 270);
            graphics.SetSmoothingMode(SmoothingModeDefault);
            
            // 恢复图形状态
            graphics.Restore(state);
            
            // 轨道背景（圆角矩形）
            GraphicsPath scaleTrackPath;
            int scaleTrackRadius = scaleSliderTrackHeight / 2;
            scaleTrackPath.AddArc(scaleSliderTrackX, scaleSliderTrackY, scaleTrackRadius * 2, scaleTrackRadius * 2, 180, 90);
            scaleTrackPath.AddArc(scaleSliderTrackX + scaleSliderTrackWidth - scaleTrackRadius * 2, scaleSliderTrackY, scaleTrackRadius * 2, scaleTrackRadius * 2, 270, 90);
            scaleTrackPath.AddArc(scaleSliderTrackX + scaleSliderTrackWidth - scaleTrackRadius * 2, scaleSliderTrackY + scaleSliderTrackHeight - scaleTrackRadius * 2, scaleTrackRadius * 2, scaleTrackRadius * 2, 0, 90);
            scaleTrackPath.AddArc(scaleSliderTrackX, scaleSliderTrackY + scaleSliderTrackHeight - scaleTrackRadius * 2, scaleTrackRadius * 2, scaleTrackRadius * 2, 90, 90);
            scaleTrackPath.CloseFigure();
            
            SolidBrush scaleTrackBrush(Color(100, 100, 100, 100));  // 深灰色轨道
            graphics.FillPath(&scaleTrackBrush, &scaleTrackPath);
            
            // 计算滑块位置 (0.5 ~ 2.0 映射到轨道)
            float scaleNormalizedPos = (g_crosshairScale - SCALE_MIN) / (SCALE_MAX - SCALE_MIN);
            int scaleThumbX = scaleSliderTrackX + (int)(scaleNormalizedPos * scaleSliderTrackWidth);
            
            // 绘制已完成部分（蓝色）
            GraphicsPath scaleFilledPath;
            scaleFilledPath.AddArc(scaleSliderTrackX, scaleSliderTrackY, scaleTrackRadius * 2, scaleTrackRadius * 2, 180, 90);
            scaleFilledPath.AddLine(scaleSliderTrackX + scaleTrackRadius, scaleSliderTrackY, scaleThumbX, scaleSliderTrackY);
            scaleFilledPath.AddLine(scaleThumbX, scaleSliderTrackY, scaleThumbX, scaleSliderTrackY + scaleSliderTrackHeight);
            scaleFilledPath.AddLine(scaleThumbX, scaleSliderTrackY + scaleSliderTrackHeight, scaleSliderTrackX + scaleTrackRadius, scaleSliderTrackY + scaleSliderTrackHeight);
            scaleFilledPath.AddArc(scaleSliderTrackX, scaleSliderTrackY + scaleSliderTrackHeight - scaleTrackRadius * 2, scaleTrackRadius * 2, scaleTrackRadius * 2, 90, 90);
            scaleFilledPath.CloseFigure();
            
            SolidBrush scaleFilledBrush(Color(255, 66, 135, 245));  // 蓝色填充
            graphics.FillPath(&scaleFilledBrush, &scaleFilledPath);
            
            // 绘制滑块按钮（圆形）
            const int scaleThumbSize = 16;
            int scaleThumbY = scaleSliderTrackY + scaleSliderTrackHeight / 2 - scaleThumbSize / 2;
            
            // 滑块阴影
            SolidBrush scaleShadowBrush(Color(40, 0, 0, 0));
            graphics.FillEllipse(&scaleShadowBrush, scaleThumbX - scaleThumbSize / 2 + 1, scaleThumbY + 1, scaleThumbSize, scaleThumbSize);
            
            // 滑块主体
            SolidBrush scaleThumbBrush(Color(255, 255, 255, 255));  // 白色滑块
            graphics.FillEllipse(&scaleThumbBrush, scaleThumbX - scaleThumbSize / 2, scaleThumbY, scaleThumbSize, scaleThumbSize);
            
            // 滑块边框
            Pen scaleThumbBorderPen(Color(100, 100, 100, 100), 1.0f);
            graphics.DrawEllipse(&scaleThumbBorderPen, scaleThumbX - scaleThumbSize / 2, scaleThumbY, scaleThumbSize, scaleThumbSize);
            
            // 绘制分隔线 - 已删除
            // Pen separatorPen(Color(255, 60, 60, 60), 1.0f);
            const int separatorY = TITLEBAR_HEIGHT + 170;
            // graphics.DrawLine(&separatorPen, 0, separatorY, rc.right, separatorY);
            
            // 绘制一体化Tab滑块切换（类似 HeroUI Pro 的 Bundles/Packs）
            const int tabY = separatorY + 10;
            const int tabHeight = 40;
            const int tabPadding = 4;  // Tab容器内边距
            
            Font tabFont(L"Microsoft YaHei UI", 11, FontStyleRegular);
            
            // 计算每个Tab的宽度和位置
            std::vector<int> tabWidths;
            int totalTabWidth = 0;
            for (size_t i = 0; i < g_tabGroups.size(); ++i) {
                RectF measureRect;
                graphics.MeasureString(g_tabGroups[i].c_str(), -1, &tabFont, PointF(0, 0), &measureRect);
                int tabWidth = (int)measureRect.Width + 40;
                tabWidths.push_back(tabWidth);
                totalTabWidth += tabWidth;
            }
            
            // 绘制整体Tab容器背景
            float containerRadius = 20.0f;
            int containerWidth = totalTabWidth + tabPadding * 2;
            int containerX = MARGIN;
            RectF containerRect((float)containerX, (float)tabY, (float)containerWidth, (float)tabHeight);
            
            GraphicsPath containerPath;
            containerPath.AddArc(containerRect.X, containerRect.Y, containerRadius, containerRadius, 180.0f, 90.0f);
            containerPath.AddArc(containerRect.X + containerRect.Width - containerRadius, containerRect.Y, containerRadius, containerRadius, 270.0f, 90.0f);
            containerPath.AddArc(containerRect.X + containerRect.Width - containerRadius, containerRect.Y + containerRect.Height - containerRadius, containerRadius, containerRadius, 0.0f, 90.0f);
            containerPath.AddArc(containerRect.X, containerRect.Y + containerRect.Height - containerRadius, containerRadius, containerRadius, 90.0f, 90.0f);
            containerPath.CloseFigure();
            
            // 容器背景：oklab(0 none none/.3) = 30%透明度的黑色
            SolidBrush containerBrush(Color(76, 0, 0, 0));  // 30% alpha
            graphics.FillPath(&containerBrush, &containerPath);
            
            // 绘制容器描边：oklab(100% 0 5.96046e-8 / .05) = 5%透明度的白色
            Pen containerBorderPen(Color(13, 255, 255, 255), 1.0f);  // 5% alpha white
            graphics.DrawPath(&containerBorderPen, &containerPath);
            
            // 绘制选中Tab的滑动背景高亮
            // 确保索引有效
            if (g_currentTabIndex >= 0 && g_currentTabIndex < tabWidths.size()) {
            int sliderX = containerX + tabPadding;
            for (int i = 0; i < g_currentTabIndex && i < tabWidths.size(); ++i) {
                sliderX += tabWidths[i];
            }
            
            float sliderRadius = 16.0f;
            int sliderWidth = tabWidths[g_currentTabIndex];
            RectF sliderRect((float)sliderX, (float)(tabY + tabPadding), (float)sliderWidth, (float)(tabHeight - tabPadding * 2));
            
            GraphicsPath sliderPath;
            sliderPath.AddArc(sliderRect.X, sliderRect.Y, sliderRadius, sliderRadius, 180.0f, 90.0f);
            sliderPath.AddArc(sliderRect.X + sliderRect.Width - sliderRadius, sliderRect.Y, sliderRadius, sliderRadius, 270.0f, 90.0f);
            sliderPath.AddArc(sliderRect.X + sliderRect.Width - sliderRadius, sliderRect.Y + sliderRect.Height - sliderRadius, sliderRadius, sliderRadius, 0.0f, 90.0f);
            sliderPath.AddArc(sliderRect.X, sliderRect.Y + sliderRect.Height - sliderRadius, sliderRadius, sliderRadius, 90.0f, 90.0f);
            sliderPath.CloseFigure();
            
            // 滑块背景：深蓝灰色 hsl(240 5.26% 26.08%)
            SolidBrush sliderBrush(Color(255, 66, 66, 70));
            graphics.FillPath(&sliderBrush, &sliderPath);
            }
            
            // 绘制Tab文字
            int tabX = containerX + tabPadding;
            for (size_t i = 0; i < g_tabGroups.size(); ++i) {
                RectF tabRect((float)tabX, (float)tabY, (float)tabWidths[i], (float)tabHeight);
                
                // Tab文字颜色 - 选中和未选中有不同颜色
                Color textColor = (i == g_currentTabIndex) 
                    ? Color(255, 255, 255, 255)    // 选中：纯白色
                    : Color(255, 161, 161, 170);   // 未选中：hsl(240 5.03% 64.9%) 灰蓝色
                SolidBrush textBrush(textColor);
                StringFormat format;
                format.SetAlignment(StringAlignmentCenter);
                format.SetLineAlignment(StringAlignmentCenter);
                graphics.DrawString(g_tabGroups[i].c_str(), -1, &tabFont, tabRect, &format, &textBrush);
                
                tabX += tabWidths[i];
            }
            
            // 字体设置
            HDC hdctemp = graphics.GetHDC();
            HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
            HFONT hOldFont = (HFONT)SelectObject(hdctemp, hFont);
            graphics.ReleaseHDC(hdctemp);
            
            // 准心网格从Tab标签下方开始
            int groupY = tabY + tabHeight + MARGIN;  // Tab下方开始
            int itemIndex = 0;
            
            // 获取当前Tab对应的组名
            std::wstring currentGroup = g_currentTabIndex < g_tabGroups.size() ? g_tabGroups[g_currentTabIndex] : L"";
            
            for (size_t i = 0; i < g_crosshairs.size(); ++i) {
                const auto& crosshair = g_crosshairs[i];
                
                // 只绘制当前Tab对应的准心
                if (crosshair.group != currentGroup) {
                    continue;
                }
                
                // 计算位置
                int row = itemIndex / ITEMS_PER_ROW;
                int col = itemIndex % ITEMS_PER_ROW;
                int x = MARGIN + col * (ITEM_WIDTH + MARGIN);
                int y = groupY + row * (ITEM_HEIGHT + MARGIN);
                
                // 创建圆角矩形路径
                GraphicsPath itemPath;
                int radius = 16;  // 增大圆角，更柔和
                itemPath.AddArc(x, y, radius, radius, 180, 90);
                itemPath.AddArc(x + ITEM_WIDTH - radius, y, radius, radius, 270, 90);
                itemPath.AddArc(x + ITEM_WIDTH - radius, y + ITEM_HEIGHT - radius, radius, radius, 0, 90);
                itemPath.AddArc(x, y + ITEM_HEIGHT - radius, radius, radius, 90, 90);
                itemPath.CloseFigure();
                
                // 绘制边框和选中状态（参考 HeroUI Pro 定价页面）
                // 如果是自定义准心且鼠标悬停，不绘制默认边框（会在后面绘制红色边框）
                bool isCustomHovered = (IsCustomCrosshair(i) && g_hoveredCrosshairIndex == i);
                
                if (i == g_currentIndex) {
                    // 选中态：深色背景 rgba(255, 255, 255, 0.05)
                    SolidBrush selectedBgBrush(Color(13, 255, 255, 255));  // 5% 白色半透明
                    graphics.FillPath(&selectedBgBrush, &itemPath);
                    
                    // 选中态：紫色边框高亮（Most Popular 风格）rgba(168, 85, 247, 0.6)
                    // 如果自定义准心悬停，不绘制紫色边框（会绘制红色边框）
                    if (!isCustomHovered) {
                    Pen selectedPen(Color(153, 168, 85, 247), 1.0f);  // 紫色边框，1px 与未选中一致
                    graphics.DrawPath(&selectedPen, &itemPath);
                    }
                } else {
                    // 未选中态：深色半透明背景 rgba(255, 255, 255, 0.02)
                    SolidBrush bgBrush(Color(5, 255, 255, 255));  // 2% 白色半透明
                    graphics.FillPath(&bgBrush, &itemPath);
                    
                    // 未选中态：淡白色边框 rgba(255, 255, 255, 0.1)
                    // 如果自定义准心悬停，不绘制白色边框（会绘制红色边框）
                    if (!isCustomHovered) {
                    Pen normalPen(Color(25, 255, 255, 255), 1.0f);  // 10% 白色
                    graphics.DrawPath(&normalPen, &itemPath);
                    }
                }
                
                // 绘制准心图片（应用颜色）
                Image* img = Image::FromFile(crosshair.path.c_str());
                if (img && img->GetLastStatus() == Ok) {
                    int imgW = img->GetWidth();
                    int imgH = img->GetHeight();
                    float scale = std::min((float)(ITEM_WIDTH - 24) / imgW, (float)(ITEM_HEIGHT - 44) / imgH);
                    int drawW = (int)(imgW * scale);
                    int drawH = (int)(imgH * scale);
                    int imgX = x + (ITEM_WIDTH - drawW) / 2;
                    int imgY = y + 12 + (ITEM_HEIGHT - 44 - drawH) / 2;
                    
                    // 如果是自定义准心，不应用颜色，直接绘制原图
                    // 注意：这里使用全局索引 i，而不是 itemIndex
                    if (IsCustomCrosshair(i)) {
                        graphics.DrawImage(img, imgX, imgY, drawW, drawH);
                    } else {
                    // 使用带颜色的绘制方法
                    DrawColoredImage(graphics, img, imgX, imgY, drawW, drawH, 
                        g_colors[g_currentColorIndex].color);
                    }
                    delete img;
                }
                
                
                // 绘制名称
                std::wstring shortName = crosshair.name;
                if (shortName.length() > 10) {
                    shortName = shortName.substr(0, 9) + L"...";
                }
                
                // 名称文本 - 白色
                Font nameFont(L"Microsoft YaHei UI", 9, FontStyleBold);
                SolidBrush nameBrush(Color(255, 220, 220, 220));
                RectF nameRect((float)x, (float)(y + ITEM_HEIGHT - 27), (float)ITEM_WIDTH, 20.0f);  // 向上移动2px
                StringFormat format;
                format.SetAlignment(StringAlignmentCenter);
                format.SetLineAlignment(StringAlignmentCenter);
                graphics.DrawString(shortName.c_str(), -1, &nameFont, nameRect, &format, &nameBrush);
                
                itemIndex++;
            }
            
            // 注意：此处不再绘制上传按钮，已移除上传功能
            
            // 如果是"自定义"分组，在左下角绘制提示文字
            if (currentGroup == L"自定义") {
                Font hintFont(L"Microsoft YaHei UI", 9, FontStyleRegular);
                SolidBrush hintBrush(Color(178, 150, 150, 150));  // 灰色文字，70%不透明度 (178/255 ≈ 0.7)
                
                std::wstring hintText = L"* 请将自定义准心图片放在resource文件夹";
                RectF hintRect((float)MARGIN, (float)(rc.bottom - 35), (float)(rc.right - MARGIN * 2), 25.0f);
                
                StringFormat hintFormat;
                hintFormat.SetAlignment(StringAlignmentNear);  // 左对齐
                hintFormat.SetLineAlignment(StringAlignmentCenter);
                
                graphics.DrawString(hintText.c_str(), -1, &hintFont, hintRect, &hintFormat, &hintBrush);
            }
            
            SelectObject(hdctemp, hOldFont);
            DeleteObject(hFont);
            
            // 绘制黑色边框（覆盖系统边框）
            Pen blackBorderPen(Color(255, 0, 0, 0), 1.0f);  // 纯黑色边框
            graphics.DrawRectangle(&blackBorderPen, 0, 0, rc.right - 1, rc.bottom - 1);
            
            // 将内存DC的内容复制到屏幕DC - 完成双缓冲
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);
            
            // 清理双缓冲资源
            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        
        case WM_ERASEBKGND: {
            // 不擦除背景，避免闪烁（双缓冲会绘制所有内容）
            return 1;
        }
        
        case WM_TIMER: {
            // Switch动画已移除，不再处理 TIMER_SWITCH_ANIM
            if (wParam == TIMER_RESET_ANIM) {
                // 重置图标旋转动画更新
                if (g_resetIconAnimating) {
                    g_resetIconRotation += RESET_ROTATION_SPEED;
                    
                    // 完成一圈旋转（360度）
                    if (g_resetIconRotation >= 360.0f) {
                        g_resetIconRotation = 0.0f;
                        g_resetIconAnimating = false;
                        KillTimer(hwnd, TIMER_RESET_ANIM);
                    }
                    
                    // 重绘窗口
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            int xPos = LOWORD(lParam);
            int yPos = HIWORD(lParam);
            
            // 获取窗口大小
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            // 如果正在拖拽滑块
            if (g_isDraggingScaleSlider) {
                const int scaleSliderTrackWidth = 200;
                const int scaleSliderTrackX = MARGIN + 60;
                
                // 计算新的缩放值
                float newNormalizedPos = (float)(xPos - scaleSliderTrackX) / scaleSliderTrackWidth;
                newNormalizedPos = std::max(0.0f, std::min(1.0f, newNormalizedPos));
                g_crosshairScale = SCALE_MIN + newNormalizedPos * (SCALE_MAX - SCALE_MIN);
                
                // 更新准心大小
                AdjustCrosshairScale(0);  // 触发更新，delta为0只刷新显示
                
                return 0;
            }
            
            // 检测鼠标是否在关闭按钮上
            const int closeButtonSize = 40;
            const int closeButtonMargin = 8;
            const int closeButtonX = rc.right - closeButtonMargin - closeButtonSize;
            const int closeButtonY = 0;
            
            bool wasHover = g_bCloseButtonHover;
            g_bCloseButtonHover = (xPos >= closeButtonX && xPos <= closeButtonX + closeButtonSize &&
                                   yPos >= closeButtonY && yPos <= closeButtonY + closeButtonSize);
            
            // 如果悬停状态改变，重绘标题栏区域
            if (wasHover != g_bCloseButtonHover) {
                RECT titleRect = {0, 0, rc.right, TITLEBAR_HEIGHT};
                InvalidateRect(hwnd, &titleRect, FALSE);
            }
            
            // 检测鼠标是否在自定义准心卡片上
            const int separatorY = TITLEBAR_HEIGHT + 170;
            const int tabY = separatorY + 10;
            const int tabHeight = 40;
            int groupY = tabY + tabHeight + MARGIN;
            
            std::wstring currentGroup = g_currentTabIndex < g_tabGroups.size() ? g_tabGroups[g_currentTabIndex] : L"";
            int itemIndex = 0;
            int newHoveredIndex = -1;
            
            for (size_t i = 0; i < g_crosshairs.size(); ++i) {
                const auto& crosshair = g_crosshairs[i];
                if (crosshair.group != currentGroup) continue;
                
                int row = itemIndex / ITEMS_PER_ROW;
                int col = itemIndex % ITEMS_PER_ROW;
                int x = MARGIN + col * (ITEM_WIDTH + MARGIN);
                int y = groupY + row * (ITEM_HEIGHT + MARGIN);
                
                // 检查是否在准心卡片上
                if (xPos >= x && xPos <= x + ITEM_WIDTH &&
                    yPos >= y && yPos <= y + ITEM_HEIGHT) {
                    // 如果是自定义准心，设置悬停状态
                    if (IsCustomCrosshair(i)) {
                        newHoveredIndex = i;
                    }
                    break;
                }
                itemIndex++;
            }
            
            // 检查悬停状态是否改变
            if (g_hoveredCrosshairIndex != newHoveredIndex) {
                g_hoveredCrosshairIndex = newHoveredIndex;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            
            // 跟踪鼠标离开事件
            TRACKMOUSEEVENT tme = {0};
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            
            return 0;
        }
        
        case WM_MOUSELEAVE: {
            // 鼠标离开窗口，取消悬停状态
            bool needRedraw = false;
            
            if (g_bCloseButtonHover) {
                g_bCloseButtonHover = false;
                needRedraw = true;
            }
            
            if (g_hoveredCrosshairIndex != -1) {
                g_hoveredCrosshairIndex = -1;
                needRedraw = true;
            }
            
            
            if (needRedraw) {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            int xPos = LOWORD(lParam);
            int yPos = HIWORD(lParam);
            
            // 获取窗口大小
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            // 检测是否点击了关闭按钮
            const int closeButtonSize = 40;
            const int closeButtonMargin = 8;  // 关闭按钮专用较小边距
            const int closeButtonX = rc.right - closeButtonMargin - closeButtonSize;
            const int closeButtonY = 0;
            
            if (xPos >= closeButtonX && xPos <= closeButtonX + closeButtonSize &&
                yPos >= closeButtonY && yPos <= closeButtonY + closeButtonSize) {
                // 点击了关闭按钮
                HidePreviewWindow();
                return 0;
            }
            
            // 检测是否点击了Tab标签（一体化滑块）
            const int separatorY = TITLEBAR_HEIGHT + 170;
            const int tabY = separatorY + 10;
            const int tabHeight = 40;
            const int tabPadding = 4;
            int tabX = MARGIN + tabPadding;
            
            Font tabFont(L"Microsoft YaHei UI", 11, FontStyleRegular);
            Graphics graphics(GetDC(hwnd));
            
            // 计算Tab容器宽度
            int totalTabWidth = 0;
            for (size_t i = 0; i < g_tabGroups.size(); ++i) {
                RectF measureRect;
                graphics.MeasureString(g_tabGroups[i].c_str(), -1, &tabFont, PointF(0, 0), &measureRect);
                totalTabWidth += (int)measureRect.Width + 40;
            }
            int containerWidth = totalTabWidth + tabPadding * 2;
            int containerX = MARGIN;
            
            for (size_t i = 0; i < g_tabGroups.size(); ++i) {
                RectF measureRect;
                graphics.MeasureString(g_tabGroups[i].c_str(), -1, &tabFont, PointF(0, 0), &measureRect);
                int tabWidth = (int)measureRect.Width + 40;
                
                if (xPos >= tabX && xPos <= tabX + tabWidth &&
                    yPos >= tabY && yPos <= tabY + tabHeight) {
                    // 切换Tab
                    g_currentTabIndex = i;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                
                tabX += tabWidth;  // 一体化布局，无间距
            }
            
            // 检测是否点击了Switch滑块（右侧）
            const int switchWidth = 50;   // 与绘制时一致
            const int switchHeight = 26;  // 与绘制时一致
            const int switchX = rc.right - MARGIN - switchWidth;  // 右对齐
            const int switchY = TITLEBAR_HEIGHT + 10;  // 与绘制时一致
            
            if (xPos >= switchX && xPos <= switchX + switchWidth &&
                yPos >= switchY && yPos <= switchY + switchHeight) {
                // 切换准心显示/隐藏
                SendMessage(g_hwndCrosshair, WM_COMMAND, IDM_TOGGLE, 0);
                
                // 直接刷新显示（无动画）
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            
            // 检测是否点击了颜色圆形 - 一行显示
            const int COLOR_SIZE = 26;
            const int COLOR_SPACING = 12;
            int colorStartX = MARGIN + 60;
            int colorStartY = TITLEBAR_HEIGHT + 77;
            
            for (size_t i = 0; i < g_colors.size(); ++i) {
                int colorX = colorStartX + i * (COLOR_SIZE + COLOR_SPACING);
                int colorY = colorStartY;
                
                // 计算圆心
                int centerX = colorX + COLOR_SIZE / 2;
                int centerY = colorY + COLOR_SIZE / 2;
                
                // 计算点击点与圆心的距离
                int dx = xPos - centerX;
                int dy = yPos - centerY;
                int distanceSquared = dx * dx + dy * dy;
                int radiusSquared = (COLOR_SIZE / 2) * (COLOR_SIZE / 2);
                
                // 如果在圆内
                if (distanceSquared <= radiusSquared) {
                    // 如果是自定义准心，不应用颜色配置
                    if (IsCustomCrosshair(g_currentIndex)) {
                        return 0;  // 忽略颜色选择
                    }
                    
                    // 选中这个颜色
                    g_currentColorIndex = i;
                    // 保存配置
                    SaveConfig();
                    // 立即刷新预览窗口显示新颜色
                    InvalidateRect(hwnd, nullptr, TRUE);
                    UpdateWindow(hwnd);
                    // 应用颜色到准心
                    ApplyColorToCrosshair();
                    return 0;
                }
            }
            
            // 检测是否点击了重置图标
            const int scaleSliderTrackWidth = 200;
            const int scaleSliderTrackX = MARGIN + 60;
            const int scaleTextX = scaleSliderTrackX + scaleSliderTrackWidth + 10;
            const int scaleTextY = TITLEBAR_HEIGHT + 122;
            const int resetIconSize = 12;  // 更新为缩小后的尺寸
            const int resetIconX = scaleTextX + 48;
            const int resetIconY = scaleTextY + 4;
            
            // 检测点击区域（包含一定的触摸余量）
            if (xPos >= resetIconX - 4 && xPos <= resetIconX + resetIconSize + 4 &&
                yPos >= resetIconY - 4 && yPos <= resetIconY + resetIconSize + 4) {
                // 重置缩放为100%
                g_crosshairScale = 1.0f;
                AdjustCrosshairScale(0);  // 触发更新
                
                // 启动旋转动画
                g_resetIconRotation = 0.0f;
                g_resetIconAnimating = true;
                SetTimer(hwnd, TIMER_RESET_ANIM, 16, nullptr);  // ~60fps
                
                return 0;
            }
            
            // 检测是否点击了缩放滑块
            const int scaleSliderTrackHeight = 4;
            const int scaleSliderTrackY = TITLEBAR_HEIGHT + 128;
            const int scaleThumbSize = 16;
            int scaleThumbY = scaleSliderTrackY + scaleSliderTrackHeight / 2 - scaleThumbSize / 2;
            
            // 计算当前滑块位置
            float scaleNormalizedPos = (g_crosshairScale - SCALE_MIN) / (SCALE_MAX - SCALE_MIN);
            int scaleThumbX = scaleSliderTrackX + (int)(scaleNormalizedPos * scaleSliderTrackWidth);
            
            // 检测是否点击滑块（圆形区域）
            int scaleThumbCenterX = scaleThumbX;
            int scaleThumbCenterY = scaleThumbY + scaleThumbSize / 2;
            int scaleDx = xPos - scaleThumbCenterX;
            int scaleDy = yPos - scaleThumbCenterY;
            
            if (scaleDx * scaleDx + scaleDy * scaleDy <= (scaleThumbSize / 2) * (scaleThumbSize / 2)) {
                // 开始拖拽滑块
                g_isDraggingScaleSlider = true;
                SetCapture(hwnd);  // 捕获鼠标
                return 0;
            }
            
            // 检测是否点击轨道（直接跳转）
            if (xPos >= scaleSliderTrackX && xPos <= scaleSliderTrackX + scaleSliderTrackWidth &&
                yPos >= scaleSliderTrackY - 10 && yPos <= scaleSliderTrackY + scaleSliderTrackHeight + 10) {
                // 点击轨道，直接跳转到该位置
                float newScaleNormalizedPos = (float)(xPos - scaleSliderTrackX) / scaleSliderTrackWidth;
                newScaleNormalizedPos = std::max(0.0f, std::min(1.0f, newScaleNormalizedPos));
                g_crosshairScale = SCALE_MIN + newScaleNormalizedPos * (SCALE_MAX - SCALE_MIN);
                AdjustCrosshairScale(0);  // 触发更新
                g_isDraggingScaleSlider = true;
                SetCapture(hwnd);
                return 0;
            }
            
            // 检测点击了哪个准心
            int groupY = tabY + tabHeight + MARGIN;  // Tab下方开始
            int itemIndex = 0;
            
            // 获取当前Tab对应的组名
            std::wstring currentGroup = g_currentTabIndex < g_tabGroups.size() ? g_tabGroups[g_currentTabIndex] : L"";
            
            for (size_t i = 0; i < g_crosshairs.size(); ++i) {
                const auto& crosshair = g_crosshairs[i];
                
                // 只检测当前Tab对应的准心
                if (crosshair.group != currentGroup) {
                    continue;
                }
                
                int row = itemIndex / ITEMS_PER_ROW;
                int col = itemIndex % ITEMS_PER_ROW;
                int x = MARGIN + col * (ITEM_WIDTH + MARGIN);
                int y = groupY + row * (ITEM_HEIGHT + MARGIN);
                
                if (xPos >= x && xPos <= x + ITEM_WIDTH &&
                    yPos >= y && yPos <= y + ITEM_HEIGHT) {
                    // 单次点击：选中这个准心（无论是否自定义）
                    SendMessage(g_hwndCrosshair, WM_PREVIEW_SELECT, i, 0);
                    return 0;
                }
                
                itemIndex++;
            }
            
            
            return 0;
        }
        
        case WM_RBUTTONDOWN: {
            // 右键点击：如果是自定义准心，显示删除菜单
            int xPos = LOWORD(lParam);
            int yPos = HIWORD(lParam);
            
            // 获取窗口大小
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            // 检测点击了哪个准心
            const int separatorY = TITLEBAR_HEIGHT + 170;
            const int tabY = separatorY + 10;
            const int tabHeight = 40;
            int groupY = tabY + tabHeight + MARGIN;  // Tab下方开始
            int itemIndex = 0;
            
            // 获取当前Tab对应的组名
            std::wstring currentGroup = g_currentTabIndex < g_tabGroups.size() ? g_tabGroups[g_currentTabIndex] : L"";
            
            for (size_t i = 0; i < g_crosshairs.size(); ++i) {
                const auto& crosshair = g_crosshairs[i];
                
                // 只检测当前Tab对应的准心
                if (crosshair.group != currentGroup) {
                    continue;
                }
                
                int row = itemIndex / ITEMS_PER_ROW;
                int col = itemIndex % ITEMS_PER_ROW;
                int x = MARGIN + col * (ITEM_WIDTH + MARGIN);
                int y = groupY + row * (ITEM_HEIGHT + MARGIN);
                
                if (xPos >= x && xPos <= x + ITEM_WIDTH &&
                    yPos >= y && yPos <= y + ITEM_HEIGHT) {
                    // 右键点击自定义准心时显示删除菜单
                    if (IsCustomCrosshair(i)) {
                        g_rightClickedCrosshairIndex = i;
                        
                        // 显示自定义暗色菜单
                        POINT pt = {xPos, yPos};
                        ClientToScreen(hwnd, &pt);
                        ShowDeleteMenu(hwnd, pt.x, pt.y);
                        return 0;
                    }
                }
                
                itemIndex++;
            }
            
            return 0;
        }
        
        case WM_LBUTTONUP: {
            // 释放鼠标按键，结束拖拽
            if (g_isDraggingScaleSlider) {
                g_isDraggingScaleSlider = false;
                ReleaseCapture();  // 释放鼠标捕获
            }
            return 0;
        }
        
        case WM_NCHITTEST: {
            // 允许通过标题栏拖动窗口
            LRESULT hit = DefWindowProc(hwnd, msg, wParam, lParam);
            if (hit == HTCLIENT) {
                POINT pt = {LOWORD(lParam), HIWORD(lParam)};
                ScreenToClient(hwnd, &pt);
                
                // 如果点击在标题栏区域（不包括关闭按钮）
                RECT rc;
                GetClientRect(hwnd, &rc);
                const int closeButtonSize = 40;
                const int closeButtonMargin = 8;  // 关闭按钮专用较小边距
                const int closeButtonX = rc.right - closeButtonMargin - closeButtonSize;
                
                if (pt.y >= 0 && pt.y <= TITLEBAR_HEIGHT && 
                    !(pt.x >= closeButtonX && pt.x <= closeButtonX + closeButtonSize)) {
                    return HTCAPTION;  // 可拖动
                }
            }
            return hit;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            
            if (wmId == IDM_DELETE_CROSSHAIR) {
                // 删除右键点击的准心
                if (g_rightClickedCrosshairIndex >= 0 && g_rightClickedCrosshairIndex < g_crosshairs.size()) {
                    DeleteCustomCrosshair(g_rightClickedCrosshairIndex);
                    g_rightClickedCrosshairIndex = -1;
                }
                return 0;
            }
            
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        
        case WM_CLOSE:
            HidePreviewWindow();
            return 0;
            
        case WM_DESTROY:
            // Switch动画已移除，不再需要清理定时器
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 创建预览窗口
void CreatePreviewWindow() {
    if (g_hwndPreview) return;
    
    int width = 4 * 120 + 5 * 22 + 40;  // 4列 + 边距（22px）+ 额外宽度
    int height = 650;
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;
    
    g_hwndPreview = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED,  // 添加分层窗口样式
        PREVIEW_CLASS_NAME,
        L"",  // 无标题
        WS_POPUP,  // 纯无边框窗口
        x, y, width, height,
        nullptr,
        nullptr,
        g_hInst,
        nullptr
    );
    
    // 添加窗口阴影和圆角
    if (g_hwndPreview) {
        // 设置窗口圆角（Windows 11）
        int corner = 2;  // DWMWCP_ROUND
        DwmSetWindowAttribute(g_hwndPreview, 33, &corner, sizeof(corner));
        
        // 完全移除窗口边框 - 不扩展边框区域
        MARGINS margins = {0, 0, 0, 0};
        DwmExtendFrameIntoClientArea(g_hwndPreview, &margins);
        
        // 设置窗口为完全不透明
        SetLayeredWindowAttributes(g_hwndPreview, 0, 255, LWA_ALPHA);
        
        // 禁用窗口阴影（可能是阴影导致的边框感）
        BOOL useShadow = FALSE;
        DwmSetWindowAttribute(g_hwndPreview, DWMWA_NCRENDERING_ENABLED, &useShadow, sizeof(useShadow));
    }
}

// 显示预览窗口
void ShowPreviewWindow() {
    if (!g_hwndPreview) {
        CreatePreviewWindow();
    }
    if (g_hwndPreview) {
        ShowWindow(g_hwndPreview, SW_SHOW);
        SetForegroundWindow(g_hwndPreview);
        g_bPreviewWindowVisible = true;
    }
}

// 隐藏预览窗口
void HidePreviewWindow() {
    if (g_hwndPreview) {
        ShowWindow(g_hwndPreview, SW_HIDE);
        g_bPreviewWindowVisible = false;
    }
}

// 绘制带颜色的图片
void DrawColoredImage(Graphics& graphics, Image* img, int x, int y, int width, int height, const Color& tintColor) {
    if (!img) return;
    
    // 如果是原始颜色（白色），直接绘制
    if (tintColor.GetR() == 255 && tintColor.GetG() == 255 && tintColor.GetB() == 255) {
        graphics.DrawImage(img, x, y, width, height);
        return;
    }
    
    // 创建临时位图来处理颜色
    Bitmap* tempBitmap = new Bitmap(img->GetWidth(), img->GetHeight(), PixelFormat32bppARGB);
    Graphics tempGraphics(tempBitmap);
    tempGraphics.DrawImage(img, 0, 0, img->GetWidth(), img->GetHeight());
    
    // 逐像素处理，保持亮度但改变色相
    BitmapData* bitmapData = new BitmapData();
    Rect rect(0, 0, tempBitmap->GetWidth(), tempBitmap->GetHeight());
    tempBitmap->LockBits(&rect, ImageLockModeRead | ImageLockModeWrite, PixelFormat32bppARGB, bitmapData);
    
    BYTE* pixels = (BYTE*)bitmapData->Scan0;
    int stride = bitmapData->Stride;
    
    for (int y = 0; y < bitmapData->Height; y++) {
        for (int x = 0; x < bitmapData->Width; x++) {
            int index = y * stride + x * 4;
            BYTE b = pixels[index];
            BYTE g = pixels[index + 1];
            BYTE r = pixels[index + 2];
            BYTE a = pixels[index + 3];
            
            if (a > 0) {
                // 计算最大通道值作为强度（适用于黑白准心）
                BYTE maxChannel = std::max(std::max(r, g), b);
                float intensity = maxChannel / 255.0f;
                
                // 检测是黑色准心还是白色准心
                // 如果最大通道值小于128，说明是黑色准心（深色），需要反转
                if (maxChannel < 128) {
                    intensity = 1.0f - intensity;  // 反转：黑色变成1，接近黑色的变大
                }
                
                // 对于半透明像素（抗锯齿边缘），减少黑色影响
                if (a < 255) {
                    // 半透明像素：提高亮度，减少黑边
                    float alphaFactor = a / 255.0f;
                    intensity = std::max(intensity, 0.3f + (1.0f - alphaFactor) * 0.5f);
                }
                
                // 应用目标颜色
                pixels[index] = (BYTE)(tintColor.GetB() * intensity);     // B
                pixels[index + 1] = (BYTE)(tintColor.GetG() * intensity); // G
                pixels[index + 2] = (BYTE)(tintColor.GetR() * intensity); // R
                // Alpha保持不变
            }
        }
    }
    
    tempBitmap->UnlockBits(bitmapData);
    delete bitmapData;
    
    // 绘制处理后的图片
    graphics.DrawImage(tempBitmap, x, y, width, height);
    delete tempBitmap;
}

// 应用颜色到准心
void ApplyColorToCrosshair() {
    if (g_hwndCrosshair) {
        InvalidateRect(g_hwndCrosshair, nullptr, TRUE);
        UpdateWindow(g_hwndCrosshair);
    }
    
    // 如果预览窗口打开，也刷新它
    if (g_hwndPreview && g_bPreviewWindowVisible) {
        InvalidateRect(g_hwndPreview, nullptr, TRUE);
        UpdateWindow(g_hwndPreview);
    }
    
    // 不再显示通知，避免打扰用户
}

// 缓动函数 - EaseInOutCubic（三次方缓动）
// 提供平滑的加速和减速效果
float EaseInOutCubic(float t) {
    if (t < 0.5f) {
        return 4.0f * t * t * t;  // 前半段加速
    } else {
        float f = (2.0f * t - 2.0f);
        return 0.5f * f * f * f + 1.0f;  // 后半段减速
    }
}

// 调整准心缩放
void AdjustCrosshairScale(float delta) {
    g_crosshairScale += delta;
    
    // 限制范围
    if (g_crosshairScale < SCALE_MIN) g_crosshairScale = SCALE_MIN;
    if (g_crosshairScale > SCALE_MAX) g_crosshairScale = SCALE_MAX;
    
    // 如果有准心，更新窗口大小
    if (g_pCurrentImage) {
        int width = (int)(g_pCurrentImage->GetWidth() * g_crosshairScale);
        int height = (int)(g_pCurrentImage->GetHeight() * g_crosshairScale);
        SetWindowPos(g_hwndCrosshair, HWND_TOPMOST, 0, 0, width, height, 
            SWP_NOMOVE | SWP_NOACTIVATE);
        
        // 居中窗口
        CenterWindow(g_hwndCrosshair);
        
        // 重绘准心
        InvalidateRect(g_hwndCrosshair, nullptr, TRUE);
        UpdateWindow(g_hwndCrosshair);
    }
    
    // 如果预览窗口打开，刷新显示
    if (g_hwndPreview && g_bPreviewWindowVisible) {
        InvalidateRect(g_hwndPreview, nullptr, FALSE);
    }
    
    // 保存配置
    SaveConfig();
}

// 保存配置到INI文件
void SaveConfig() {
    if (g_configPath.empty()) return;
    
    // 保存当前准心索引
    WritePrivateProfileString(L"Settings", L"CurrentIndex", 
        std::to_wstring(g_currentIndex).c_str(), g_configPath.c_str());
    
    // 保存当前颜色索引
    WritePrivateProfileString(L"Settings", L"ColorIndex", 
        std::to_wstring(g_currentColorIndex).c_str(), g_configPath.c_str());
    
    // 保存缩放比例（保存为百分比整数）
    WritePrivateProfileString(L"Settings", L"Scale", 
        std::to_wstring((int)(g_crosshairScale * 100)).c_str(), g_configPath.c_str());
    
    // 保存显示状态
    WritePrivateProfileString(L"Settings", L"Visible", 
        g_bVisible ? L"1" : L"0", g_configPath.c_str());
}

// 从INI文件加载配置
void LoadConfig() {
    if (g_configPath.empty()) return;
    
    // 加载当前准心索引
    int index = GetPrivateProfileInt(L"Settings", L"CurrentIndex", 0, g_configPath.c_str());
    if (index >= 0 && index < g_crosshairs.size()) {
        g_currentIndex = index;
    }
    
    // 加载当前颜色索引
    int colorIndex = GetPrivateProfileInt(L"Settings", L"ColorIndex", 0, g_configPath.c_str());
    if (colorIndex >= 0 && colorIndex < g_colors.size()) {
        g_currentColorIndex = colorIndex;
    }
    
    // 加载缩放比例
    int scale = GetPrivateProfileInt(L"Settings", L"Scale", 100, g_configPath.c_str());
    g_crosshairScale = scale / 100.0f;
    if (g_crosshairScale < SCALE_MIN) g_crosshairScale = SCALE_MIN;
    if (g_crosshairScale > SCALE_MAX) g_crosshairScale = SCALE_MAX;
    
    // 加载显示状态
    int visible = GetPrivateProfileInt(L"Settings", L"Visible", 1, g_configPath.c_str());
    g_bVisible = (visible != 0);
}

// 判断是否是自定义准心（通过文件名判断，排除内置准心）
bool IsCustomCrosshair(int index) {
    if (index < 0 || index >= g_crosshairs.size()) return false;
    
    const std::wstring& name = g_crosshairs[index].name;
    
    // 内置准心名称包含特定关键词
    if (name.find(L"丁字") != std::wstring::npos ||
        name.find(L"十字") != std::wstring::npos ||
        name.find(L"圆点") != std::wstring::npos ||
        name.find(L"小圆") != std::wstring::npos ||
        name.find(L"指示圈") != std::wstring::npos ||
        name.find(L"战术") != std::wstring::npos ||
        name.find(L"M145") != std::wstring::npos) {
        return false;  // 是内置准心
    }
    
    // 其他都是自定义准心
    return true;
}

// 删除自定义准心
void DeleteCustomCrosshair(int index) {
    if (index < 0 || index >= g_crosshairs.size()) return;
    
    // 检查是否是自定义准心
    if (!IsCustomCrosshair(index)) {
        MessageBox(g_hwndPreview,
            L"无法删除内置准心！\n\n只能删除自定义准心。",
            L"无法删除",
            MB_OK | MB_ICONWARNING);
        return;
    }
    
    // 确认删除
    int result = MessageBox(g_hwndPreview,
        (L"确定要删除准心 \"" + g_crosshairs[index].name + L"\" 吗？\n\n此操作无法撤销。").c_str(),
        L"确认删除",
        MB_YESNO | MB_ICONQUESTION);
    
    if (result != IDYES) {
        return;
    }
    
    // 获取文件路径
    std::wstring filePath = g_crosshairs[index].path;
    
    // 如果当前正在使用这个准心，先切换到其他准心
    if (g_currentIndex == index) {
        // 切换到第一个准心（如果存在）
        if (g_crosshairs.size() > 1) {
            int newIndex = (index == 0) ? 1 : 0;
            SetCrosshair(newIndex);
        } else {
            // 如果只有一个准心，隐藏它
            g_bVisible = false;
            ShowWindow(g_hwndCrosshair, SW_HIDE);
        }
    } else if (g_currentIndex > index) {
        // 如果删除的准心在当前准心之前，需要调整索引
        g_currentIndex--;
    }
    
    // 删除文件
    try {
        if (fs::exists(filePath)) {
            fs::remove(filePath);
        }
    } catch (const std::exception& e) {
        MessageBox(g_hwndPreview,
            L"删除文件失败！\n\n请检查文件是否被其他程序占用，或是否有删除权限。",
            L"错误",
            MB_OK | MB_ICONERROR);
        return;
    }
    
    // 重新加载准心列表
    g_crosshairs.clear();
    LoadCrosshairImages();
    
    // 确保索引有效
    if (g_currentIndex >= g_crosshairs.size()) {
        g_currentIndex = 0;
    }
    
    // 如果还有准心，设置当前准心
    if (!g_crosshairs.empty()) {
        SetCrosshair(g_currentIndex);
    }
    
    // 刷新预览窗口
    if (g_hwndPreview && g_bPreviewWindowVisible) {
        InvalidateRect(g_hwndPreview, nullptr, TRUE);
        UpdateWindow(g_hwndPreview);
    }
    
    // 显示系统托盘通知
    if (g_nid.hWnd) {
        g_nid.uFlags |= NIF_INFO;
        wcscpy_s(g_nid.szInfoTitle, L"删除成功");
        wcscpy_s(g_nid.szInfo, L"准心已删除");
        g_nid.dwInfoFlags = NIIF_INFO;
        Shell_NotifyIcon(NIM_MODIFY, &g_nid);
    }
}

// 显示删除菜单（使用Windows标准弹出菜单）
void ShowDeleteMenu(HWND hwnd, int x, int y) {
    POINT pt = {x, y};
    
    // 创建弹出菜单
    HMENU hMenu = CreatePopupMenu();
    
    // 添加删除选项
    AppendMenu(hMenu, MF_STRING, IDM_DELETE_CROSSHAIR, L"删除准心");
    
    // 显示菜单
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
    PostMessage(hwnd, WM_NULL, 0, 0);
    
    // 清理
    DestroyMenu(hMenu);
}

// 菜单窗口过程（虽然注册了但实际使用标准弹出菜单，保留以防将来需要）
LRESULT CALLBACK MenuWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 目前未使用，保留空实现
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

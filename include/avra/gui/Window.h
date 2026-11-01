#pragma once

#include "avra/gui/Component.h"
#include "avra/gui/Graphics.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <dwrite.h>
#endif


namespace avra::gui {

/**
 * @brief Platform window host that embeds into DAW hosts and runs Direct2D.
 */
class Window {
public:
    explicit Window(Component* rootComponent)
        : rootComponent_(rootComponent) {
#if defined(_WIN32)
        registerWindowClass();
#endif
    }

    ~Window() {
        destroy();
    }

#if defined(_WIN32)
    static HINSTANCE getCurrentModuleInstance() noexcept {
        HMODULE hMod = nullptr;
        if (GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&Window::wndProc),
                &hMod) && hMod) {
            return static_cast<HINSTANCE>(hMod);
        }
        return GetModuleHandleW(nullptr);
    }

    static const wchar_t* getPluginWindowClassName() noexcept {
        static wchar_t s_className[64] = {};
        if (s_className[0] == L'\0') {
            wsprintfW(s_className, L"Avra_Plugin_Window_%p", reinterpret_cast<void*>(getCurrentModuleInstance()));
        }
        return s_className;
    }

    bool create(HWND parentHwnd, int width, int height) {
        width_ = width;
        height_ = height;
        clientWidth_ = width;
        clientHeight_ = height;
        parentHwnd_ = parentHwnd;

        if (rootComponent_) {
            rootComponent_->setBounds(0, 0, static_cast<float>(width), static_cast<float>(height));
        }

        hwnd_ = CreateWindowExW(
            0,
            getPluginWindowClassName(),
            L"",
            WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0, 0, width, height,
            parentHwnd,
            nullptr,
            getCurrentModuleInstance(),
            this
        );

        if (!hwnd_) return false;

        if (HICON hIcon = loadApplicationIcon(0, 0)) {
            SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
            HICON hSmall = loadApplicationIcon(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
            SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hSmall ? hSmall : hIcon));
        }

        initDirect2D();

        if (rootComponent_) {
            rootComponent_->setBounds(0, 0, static_cast<float>(width), static_cast<float>(height));
            rootComponent_->setRepaintCallback([this]() {
                if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
            });
        }

        ShowWindow(hwnd_, SW_SHOW);

        // Start 30 FPS refresh timer (33 ms) for embedded DAW plugin views
        SetTimer(hwnd_, 1, 33, nullptr);
        return true;
    }

    bool createStandalone(const wchar_t* title, int width, int height) {
        width_ = width;
        height_ = height;
        clientWidth_ = width;
        clientHeight_ = height;
        isStandalone_ = true;

        const DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        RECT rc = { 0, 0, width, height };
        AdjustWindowRect(&rc, style, FALSE);

        const int winW = rc.right - rc.left;
        const int winH = rc.bottom - rc.top;
        const int posX = std::max(0, (GetSystemMetrics(SM_CXSCREEN) - winW) / 2);
        const int posY = std::max(0, (GetSystemMetrics(SM_CYSCREEN) - winH) / 2);

        hwnd_ = CreateWindowExW(
            0,
            L"AvraStudioClass",
            title ? title : L"Avra Studio",
            style,
            posX, posY, winW, winH,
            nullptr,
            nullptr,
            GetModuleHandle(nullptr),
            this
        );

        if (!hwnd_) return false;

        if (HICON hIcon = loadApplicationIcon(0, 0)) {
            SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
            HICON hSmall = loadApplicationIcon(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
            SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hSmall ? hSmall : hIcon));
        }

        initDirect2D();

        if (rootComponent_) {
            rootComponent_->setBounds(0, 0, static_cast<float>(width), static_cast<float>(height));
            rootComponent_->setRepaintCallback([this]() {
                if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
            });
        }

        SetTimer(hwnd_, 1, 16, nullptr);
        return true;
    }

    bool toggleFullscreen() {
        if (!hwnd_ || !isStandalone_) return false;

        const DWORD style = static_cast<DWORD>(GetWindowLongW(hwnd_, GWL_STYLE));
        if (!isFullscreen_) {
            MONITORINFO mi{ sizeof(mi) };
            if (GetWindowPlacement(hwnd_, &prevPlacement_) &&
                GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTOPRIMARY), &mi)) {
                isFullscreen_ = true;
                SetWindowLongW(hwnd_, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(
                    hwnd_, HWND_TOP,
                    mi.rcMonitor.left, mi.rcMonitor.top,
                    mi.rcMonitor.right - mi.rcMonitor.left,
                    mi.rcMonitor.bottom - mi.rcMonitor.top,
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED
                );
            }
        } else {
            isFullscreen_ = false;
            SetWindowLongW(hwnd_, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
            SetWindowPlacement(hwnd_, &prevPlacement_);
            SetWindowPos(
                hwnd_, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
            );
        }
        InvalidateRect(hwnd_, nullptr, FALSE);
        return isFullscreen_;
    }

    [[nodiscard]] bool isFullscreen() const noexcept { return isFullscreen_; }

    static bool toggleWindowFullscreen(HWND hwnd) {
        if (!hwnd) hwnd = GetActiveWindow();
        if (!hwnd) return false;
        auto* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (window) {
            return window->toggleFullscreen();
        }
        return false;
    }

    static bool isWindowFullscreen(HWND hwnd) {
        if (!hwnd) hwnd = GetActiveWindow();
        if (!hwnd) return false;
        auto* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        return window ? window->isFullscreen() : false;
    }

    int runMessageLoop() {
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return static_cast<int>(msg.wParam);
    }

    void destroy() {
        if (hwnd_) {
            KillTimer(hwnd_, 1);
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }
        cleanupDirect2D();
    }


    void setSize(int width, int height) {
        width_ = width;
        height_ = height;
        if (hwnd_) {
            SetWindowPos(hwnd_, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE);
            if (renderTarget_) {
                renderTarget_->Resize(D2D1::SizeU(width, height));
            }
            if (rootComponent_) {
                rootComponent_->setBounds(0, 0, static_cast<float>(width), static_cast<float>(height));
            }
        }
    }

    void show() {
        if (hwnd_) ShowWindow(hwnd_, SW_SHOW);
    }

    void hide() {
        if (hwnd_) ShowWindow(hwnd_, SW_HIDE);
    }

    [[nodiscard]] HWND getHwnd() const noexcept { return hwnd_; }
    [[nodiscard]] int getWidth() const noexcept { return width_; }
    [[nodiscard]] int getHeight() const noexcept { return height_; }

private:
    HWND hwnd_{nullptr};
    HWND parentHwnd_{nullptr};
    int width_{600};
    int height_{400};
    int clientWidth_{600};
    int clientHeight_{400};
    Component* rootComponent_{nullptr};
    bool isStandalone_{false};
    bool isFullscreen_{false};
    WINDOWPLACEMENT prevPlacement_{ sizeof(WINDOWPLACEMENT) };

    ID2D1Factory* d2dFactory_{nullptr};
    IDWriteFactory* dwriteFactory_{nullptr};
    ID2D1HwndRenderTarget* renderTarget_{nullptr};
    ID2D1SolidColorBrush* solidBrush_{nullptr};

    Component* capturedComponent_{nullptr};
    Component* focusedComponent_{nullptr};
    Component* hoveredComponent_{nullptr};
    Point<float> dragStartScreenPos_{0.0f, 0.0f};
    bool isLeftDragging_{false};
    bool isRightDragging_{false};

    [[nodiscard]] Point<float> toLogicalPos(float clientX, float clientY) const noexcept {
        const float cw = static_cast<float>(std::max(1, clientWidth_));
        const float ch = static_cast<float>(std::max(1, clientHeight_));
        const float lw = static_cast<float>(std::max(1, width_));
        const float lh = static_cast<float>(std::max(1, height_));
        return { clientX * (lw / cw), clientY * (lh / ch) };
    }

    static HICON loadApplicationIcon(int cx, int cy) noexcept {
        HINSTANCE hInst = GetModuleHandleW(nullptr);
        const UINT sharedFlag = (cx == 0 && cy == 0) ? (LR_DEFAULTSIZE | LR_SHARED) : LR_DEFAULTCOLOR;
        // 1. Try embedded resource ID 1 and 101 (IDI_ICON1 in assets/avra_studio.rc)
        HICON hIcon = static_cast<HICON>(LoadImageW(
            hInst, MAKEINTRESOURCEW(1), IMAGE_ICON, cx, cy, sharedFlag));
        if (!hIcon) {
            hIcon = static_cast<HICON>(LoadImageW(
                hInst, MAKEINTRESOURCEW(101), IMAGE_ICON, cx, cy, sharedFlag));
        }
        if (!hIcon) {
            // 2. Try string resource name "IDI_ICON1"
            hIcon = static_cast<HICON>(LoadImageW(
                hInst, L"IDI_ICON1", IMAGE_ICON, cx, cy, sharedFlag));
        }
        if (!hIcon) {
            // 3. Fallback to assets/avra_studio.ico on disk (relative to CWD or executable dir)
            const UINT fileFlag = (cx == 0 && cy == 0) ? (LR_LOADFROMFILE | LR_DEFAULTSIZE) : (LR_LOADFROMFILE | LR_DEFAULTCOLOR);
            hIcon = static_cast<HICON>(LoadImageW(
                nullptr, L"assets\\avra_studio.ico", IMAGE_ICON, cx, cy, fileFlag));
            if (!hIcon) {
                wchar_t exePath[MAX_PATH]{};
                if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0) {
                    std::wstring dir(exePath);
                    const auto slash = dir.find_last_of(L"\\/");
                    if (slash != std::wstring::npos) {
                        dir = dir.substr(0, slash + 1);
                        const std::wstring candidates[3] = {
                            dir + L"assets\\avra_studio.ico",
                            dir + L"..\\assets\\avra_studio.ico",
                            dir + L"..\\..\\assets\\avra_studio.ico"
                        };
                        for (const auto& icoPath : candidates) {
                            hIcon = static_cast<HICON>(LoadImageW(nullptr, icoPath.c_str(), IMAGE_ICON, cx, cy, fileFlag));
                            if (hIcon) break;
                        }
                    }
                }
            }
        }
        return hIcon;
    }

    void registerWindowClass() {
        static bool registered = false;
        if (!registered) {
            HINSTANCE hInstance = getCurrentModuleInstance();
            WNDCLASSEXW wc = { 0 };
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
            wc.lpfnWndProc = &Window::wndProc;
            wc.hInstance = hInstance;
            HICON hIcon = static_cast<HICON>(LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
            if (!hIcon) {
                hIcon = loadApplicationIcon(0, 0);
            }
            HICON hIconSm = loadApplicationIcon(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
            wc.hIcon = hIcon;
            wc.hIconSm = hIconSm ? hIconSm : hIcon;
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.lpszClassName = L"AvraStudioClass";
            RegisterClassExW(&wc);

            wc.lpszClassName = getPluginWindowClassName();
            RegisterClassExW(&wc);
            registered = true;
        }
    }

    void initDirect2D() {
        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory_);
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&dwriteFactory_));

        if (d2dFactory_ && hwnd_) {
            RECT rc;
            GetClientRect(hwnd_, &rc);
            clientWidth_ = std::max(1L, rc.right - rc.left);
            clientHeight_ = std::max(1L, rc.bottom - rc.top);
            d2dFactory_->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(), 96.0f, 96.0f),
                D2D1::HwndRenderTargetProperties(hwnd_, D2D1::SizeU(clientWidth_, clientHeight_)),
                &renderTarget_
            );

            if (renderTarget_) {
                renderTarget_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                renderTarget_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
                renderTarget_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &solidBrush_);
            }
        }
    }

    void cleanupDirect2D() {
        if (solidBrush_) { solidBrush_->Release(); solidBrush_ = nullptr; }
        if (renderTarget_) {
            Graphics::releaseRenderTargetCaches(renderTarget_);
            renderTarget_->Release();
            renderTarget_ = nullptr;
        }
        if (dwriteFactory_) { dwriteFactory_->Release(); dwriteFactory_ = nullptr; }
        if (d2dFactory_) { d2dFactory_->Release(); d2dFactory_ = nullptr; }
    }

    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        Window* window = nullptr;
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            window = static_cast<Window*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
            window->hwnd_ = hwnd;
        } else {
            window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (window) {
            return window->handleMessage(msg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_SIZE: {
                const int newW = static_cast<int>(LOWORD(lParam));
                const int newH = static_cast<int>(HIWORD(lParam));
                if (newW > 0 && newH > 0) {
                    clientWidth_ = newW;
                    clientHeight_ = newH;
                    if (renderTarget_) {
                        renderTarget_->Resize(D2D1::SizeU(static_cast<UINT32>(newW), static_cast<UINT32>(newH)));
                    }
                    InvalidateRect(hwnd_, nullptr, FALSE);
                }
                return 0;
            }

            case WM_PAINT: {
                PAINTSTRUCT ps;
                BeginPaint(hwnd_, &ps);

                if (!renderTarget_) {
                    initDirect2D();
                }

                if (renderTarget_ && solidBrush_) {
                    renderTarget_->BeginDraw();
                    const float sx = static_cast<float>(std::max(1, clientWidth_)) / static_cast<float>(std::max(1, width_));
                    const float sy = static_cast<float>(std::max(1, clientHeight_)) / static_cast<float>(std::max(1, height_));

                    Graphics g(renderTarget_, dwriteFactory_, solidBrush_, { sx, sy });

                    if (rootComponent_) {
                        rootComponent_->renderHierarchy(g);
                    }

                    renderTarget_->SetTransform(D2D1::Matrix3x2F::Identity());
                    const HRESULT hr = renderTarget_->EndDraw();
                    if (hr == D2DERR_RECREATE_TARGET) {
                        cleanupDirect2D();
                    }
                }

                EndPaint(hwnd_, &ps);
                return 0;
            }

            case WM_TIMER: {
                if (!hwnd_ || !IsWindowVisible(hwnd_) || IsIconic(hwnd_)) {
                    return 0;
                }
                if (rootComponent_) {
                    rootComponent_->tickHierarchy();
                }
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }

            case WM_ERASEBKGND:
                return 1; // Direct2D handles all background painting (avoids flicker)

            case WM_LBUTTONDBLCLK: {
                const Point<float> pt = toLogicalPos(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
                MouseEvent e{ .pos = pt, .startPos = pt, .isLeftDown = true, .isShiftDown = (wParam & MK_SHIFT) != 0 };

                if (rootComponent_) {
                    if (auto* hit = rootComponent_->hitTest(e.pos)) {
                        MouseEvent localEvent = e;
                        localEvent.pos = hit->toLocal(e.pos);
                        localEvent.startPos = localEvent.pos;
                        hit->onDoubleClick(localEvent);
                    }
                }
                return 0;
            }

            case WM_LBUTTONDOWN: {
                SetCapture(hwnd_);
                const Point<float> pt = toLogicalPos(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
                dragStartScreenPos_ = pt;
                MouseEvent e{ .pos = pt, .startPos = dragStartScreenPos_, .isLeftDown = true, .isShiftDown = (wParam & MK_SHIFT) != 0 };

                if (rootComponent_) {
                    auto* newFocus = rootComponent_->hitTest(e.pos);
                    if (focusedComponent_ && focusedComponent_ != newFocus) {
                        focusedComponent_->onFocusLost();
                    }
                    capturedComponent_ = newFocus;
                    focusedComponent_ = newFocus;
                    if (capturedComponent_) {
                        MouseEvent localEvent = e;
                        localEvent.pos = capturedComponent_->toLocal(e.pos);
                        localEvent.startPos = localEvent.pos;
                        capturedComponent_->onMouseDown(localEvent);
                    }
                }
                isLeftDragging_ = true;
                return 0;
            }

            case WM_RBUTTONDOWN: {
                SetCapture(hwnd_);
                const Point<float> pt = toLogicalPos(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
                dragStartScreenPos_ = pt;
                MouseEvent e{ .pos = pt, .startPos = dragStartScreenPos_, .isLeftDown = false, .isRightDown = true, .isShiftDown = (wParam & MK_SHIFT) != 0 };

                if (rootComponent_) {
                    auto* newFocus = rootComponent_->hitTest(e.pos);
                    capturedComponent_ = newFocus;
                    if (capturedComponent_) {
                        MouseEvent localEvent = e;
                        localEvent.pos = capturedComponent_->toLocal(e.pos);
                        localEvent.startPos = localEvent.pos;
                        capturedComponent_->onMouseDown(localEvent);
                    }
                }
                isRightDragging_ = true;
                return 0;
            }

            case WM_CHAR: {
                if (focusedComponent_) {
                    focusedComponent_->onChar(static_cast<wchar_t>(wParam));
                }
                return 0;
            }

            case WM_KEYDOWN: {
                if (wParam == VK_F11) {
                    toggleFullscreen();
                    return 0;
                }
                const bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                const bool isFunctionKey = (wParam >= VK_F1 && wParam <= VK_F12) || wParam == VK_ESCAPE;
                if (ctrlDown || isFunctionKey) {
                    if (rootComponent_) {
                        rootComponent_->onKeyDown(static_cast<int>(wParam));
                    }
                    return 0;
                }
                if (focusedComponent_ && focusedComponent_->isTextInputFocused()) {
                    focusedComponent_->onKeyDown(static_cast<int>(wParam));
                } else if (rootComponent_) {
                    rootComponent_->onKeyDown(static_cast<int>(wParam));
                }
                return 0;
            }

            case WM_CLOSE: {
                if (rootComponent_ && !rootComponent_->canCloseWindow()) {
                    return 0;
                }
                DestroyWindow(hwnd_);
                return 0;
            }

            case WM_MOUSEMOVE: {
                const Point<float> pt = toLogicalPos(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
                MouseEvent e{
                    .pos = pt,
                    .startPos = dragStartScreenPos_,
                    .isLeftDown = isLeftDragging_,
                    .isRightDown = isRightDragging_,
                    .isShiftDown = (wParam & MK_SHIFT) != 0
                };

                if ((isLeftDragging_ || isRightDragging_) && capturedComponent_) {
                    MouseEvent localEvent = e;
                    localEvent.pos = capturedComponent_->toLocal(e.pos);
                    localEvent.startPos = capturedComponent_->toLocal(dragStartScreenPos_);
                    capturedComponent_->onMouseDrag(localEvent);
                } else if (rootComponent_) {
                    auto* hovered = rootComponent_->hitTest(e.pos);
                    if (hovered != hoveredComponent_) {
                        if (hoveredComponent_) {
                            hoveredComponent_->onMouseLeave();
                        }
                        hoveredComponent_ = hovered;
                        if (hoveredComponent_) {
                            hoveredComponent_->onMouseEnter();
                        }
                    }
                    if (hovered) {
                        MouseEvent localEvent = e;
                        localEvent.pos = hovered->toLocal(e.pos);
                        hovered->onMouseMove(localEvent);
                    }
                }
                return 0;
            }

            case WM_LBUTTONUP: {
                if (isLeftDragging_) {
                    ReleaseCapture();
                    isLeftDragging_ = false;
                    const Point<float> pt = toLogicalPos(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
                    MouseEvent e{ .pos = pt, .startPos = dragStartScreenPos_, .isLeftDown = false, .isShiftDown = (wParam & MK_SHIFT) != 0 };

                    if (capturedComponent_) {
                        MouseEvent localEvent = e;
                        localEvent.pos = capturedComponent_->toLocal(e.pos);
                        localEvent.startPos = capturedComponent_->toLocal(dragStartScreenPos_);
                        capturedComponent_->onMouseUp(localEvent);
                        capturedComponent_ = nullptr;
                    }
                }
                return 0;
            }

            case WM_RBUTTONUP: {
                if (isRightDragging_) {
                    ReleaseCapture();
                    isRightDragging_ = false;
                    const Point<float> pt = toLogicalPos(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
                    MouseEvent e{ .pos = pt, .startPos = dragStartScreenPos_, .isLeftDown = false, .isRightDown = false, .isShiftDown = (wParam & MK_SHIFT) != 0 };

                    if (capturedComponent_) {
                        MouseEvent localEvent = e;
                        localEvent.pos = capturedComponent_->toLocal(e.pos);
                        localEvent.startPos = capturedComponent_->toLocal(dragStartScreenPos_);
                        capturedComponent_->onMouseUp(localEvent);
                        capturedComponent_ = nullptr;
                    }
                }
                return 0;
            }

            case WM_MOUSEWHEEL: {
                const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
                POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ScreenToClient(hwnd_, &pt);
                const Point<float> localPt = toLogicalPos(static_cast<float>(pt.x), static_cast<float>(pt.y));

                if (rootComponent_) {
                    auto* comp = rootComponent_->hitTest(localPt);
                    if (comp) {
                        comp->onMouseWheel(delta);
                    }
                }
                return 0;
            }

            case WM_DESTROY: {
                KillTimer(hwnd_, 1);
                cleanupDirect2D();
                hwnd_ = nullptr;
                if (isStandalone_) {
                    PostQuitMessage(0);
                }
                return 0;
            }
        }
        return DefWindowProcW(hwnd_, msg, wParam, lParam);
    }
#endif
};

} // namespace avra::gui

#pragma once

#ifndef NOMINMAX
#  define NOMINMAX
#endif

#include <windows.h>
#include <wrl/client.h>
#include <WebView2.h>
#include <string>

// ---------------------------------------------------------------------------
// MainWindow
//
// Win32 window hosting a WebView2 control.
//
// Improvements over v01:
//  - DPI-aware: crisp on high-resolution / 4K displays.
//  - Minimum window size enforced (900 x 600).
//  - Native Windows file-open dialog for videos (GetOpenFileName).
//  - Native Save-As dialog for CSV export (GetSaveFileName).
//  - JS -> C++ message bridge: HTML posts messages, C++ handles them
//    and replies by injecting JS back into the WebView.
// ---------------------------------------------------------------------------
class MainWindow
{
public:
    MainWindow(HINSTANCE instance, int showCommand);
    ~MainWindow();

    int run();

private:
    // Win32 plumbing.
    static LRESULT CALLBACK staticWndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT wndProc(UINT, WPARAM, LPARAM);

    bool createWindow();
    void initWebView();
    void resizeWebView();

    // JS <-> C++ bridge.
    void handleWebMessage(const std::wstring& json);
    void execScript(const std::wstring& js);

    // Native dialogs.
    std::wstring openVideoDialog();
    std::wstring saveCsvDialog();

    // Helpers.
    std::wstring exeDir()                          const;
    std::wstring htmlPath()                        const;
    std::wstring toFileUri(const std::wstring& p)  const;
    std::wstring jsEscape(const std::wstring& s)   const;
    void         showError(const std::wstring& msg) const;

    static constexpr int kIconId = 101;
    static constexpr int kInitW  = 1400;
    static constexpr int kInitH  = 960;
    static constexpr int kMinW   = 900;
    static constexpr int kMinH   = 600;

    HINSTANCE instance_{};
    int       showCommand_{};
    HWND      hwnd_{};

    bool comInitialized_{ false };

    Microsoft::WRL::ComPtr<ICoreWebView2Controller> controller_;
    Microsoft::WRL::ComPtr<ICoreWebView2>           webview_;
};

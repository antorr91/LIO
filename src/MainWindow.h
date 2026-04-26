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
// Creates a Win32 window, hosts a WebView2 control inside it, and navigates
// to assets/LIO.html sitting next to the executable.
// ---------------------------------------------------------------------------
class MainWindow
{
public:
    MainWindow(HINSTANCE instance, int showCommand);
    ~MainWindow();

    // Run the Win32 message loop. Returns the exit code.
    int run();

private:
    // Window procedure (static trampoline + member dispatch).
    static LRESULT CALLBACK staticWndProc(HWND hwnd, UINT msg,
                                          WPARAM wp, LPARAM lp);
    LRESULT wndProc(UINT msg, WPARAM wp, LPARAM lp);

    // Lifecycle helpers.
    bool createWindow();
    void initWebView();
    void resizeWebView();

    // Path helpers.
    std::wstring exeDir()      const;
    std::wstring htmlPath()    const;
    std::wstring toFileUri(const std::wstring& path) const;

    void showError(const std::wstring& msg) const;

private:
    HINSTANCE instance_{};
    int       showCommand_{};
    HWND      hwnd_{};

    bool comInitialized_{ false };

    Microsoft::WRL::ComPtr<ICoreWebView2Controller> controller_;
    Microsoft::WRL::ComPtr<ICoreWebView2>           webview_;
};

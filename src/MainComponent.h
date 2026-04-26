#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <wrl/client.h>
#include <WebView2.h>

#include <string>

class MainComponent
{
public:
    MainComponent(HINSTANCE instance, int showCommand);
    ~MainComponent();

    int run();

private:
    static LRESULT CALLBACK staticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT wndProc(UINT message, WPARAM wParam, LPARAM lParam);

    bool createMainWindow();
    void initializeWebView();
    void resizeWebView();

    std::wstring executableDirectory() const;
    std::wstring htmlPath() const;
    std::wstring fileUriFromPath(const std::wstring& path) const;

    void showError(const std::wstring& message) const;

private:
    HINSTANCE instance_{};
    int showCommand_{};
    HWND hwnd_{};

    bool comInitialized_ = false;

    Microsoft::WRL::ComPtr<ICoreWebView2Controller> controller_;
    Microsoft::WRL::ComPtr<ICoreWebView2> webview_;
};

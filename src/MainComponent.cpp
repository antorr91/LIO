#include "MainComponent.h"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <wrl.h>

using Microsoft::WRL::Callback;

namespace
{
    constexpr int LIO_ICON_ID = 101;
    constexpr const wchar_t* WindowClassName = L"LIO_WebView2_NoJUCE_Updated_Window";
}

MainComponent::MainComponent(HINSTANCE instance, int showCommand)
    : instance_(instance), showCommand_(showCommand)
{
}

MainComponent::~MainComponent()
{
    webview_.Reset();
    controller_.Reset();

    if (comInitialized_)
        CoUninitialize();
}

int MainComponent::run()
{
    if (!createMainWindow())
        return 1;

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}

bool MainComponent::createMainWindow()
{
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = MainComponent::staticWndProc;
    windowClass.hInstance = instance_;
    windowClass.lpszClassName = WindowClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIconW(instance_, MAKEINTRESOURCEW(LIO_ICON_ID));
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    RegisterClassW(&windowClass);

    hwnd_ = CreateWindowExW(
        0,
        WindowClassName,
        L"LIO - Latent Interaction Observer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1400,
        960,
        nullptr,
        nullptr,
        instance_,
        this);

    if (!hwnd_)
        return false;

    ShowWindow(hwnd_, showCommand_);
    UpdateWindow(hwnd_);
    return true;
}

LRESULT CALLBACK MainComponent::staticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    MainComponent* self = nullptr;

    if (message == WM_NCCREATE)
    {
        auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<MainComponent*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    else
    {
        self = reinterpret_cast<MainComponent*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self)
        return self->wndProc(message, wParam, lParam);

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT MainComponent::wndProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
            initializeWebView();
            return 0;

        case WM_SIZE:
            resizeWebView();
            return 0;

        case WM_DESTROY:
            webview_.Reset();
            controller_.Reset();
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(hwnd_, message, wParam, lParam);
    }
}

void MainComponent::initializeWebView()
{
    HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(comResult))
    {
        comInitialized_ = true;
    }
    else if (comResult != RPC_E_CHANGED_MODE)
    {
        showError(L"COM initialization failed.");
        return;
    }

    CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        nullptr,
        nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT environmentResult, ICoreWebView2Environment* environment) -> HRESULT
            {
                if (FAILED(environmentResult) || !environment)
                {
                    showError(L"Could not create WebView2 environment. Install Microsoft Edge WebView2 Runtime if needed.");
                    return S_OK;
                }

                environment->CreateCoreWebView2Controller(
                    hwnd_,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT controllerResult, ICoreWebView2Controller* controller) -> HRESULT
                        {
                            if (FAILED(controllerResult) || !controller)
                            {
                                showError(L"Could not create WebView2 controller.");
                                return S_OK;
                            }

                            controller_ = controller;
                            controller_->get_CoreWebView2(&webview_);
                            resizeWebView();

                            Microsoft::WRL::ComPtr<ICoreWebView2Settings> settings;
                            if (SUCCEEDED(webview_->get_Settings(&settings)) && settings)
                            {
                                settings->put_IsScriptEnabled(TRUE);
                                settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                                settings->put_IsWebMessageEnabled(TRUE);
                                settings->put_AreDevToolsEnabled(TRUE);
                                settings->put_AreDefaultContextMenusEnabled(TRUE);
                            }

                            const std::wstring htmlFile = htmlPath();

                            if (!std::filesystem::exists(htmlFile))
                            {
                                showError(L"Cannot find assets\\LIO.html next to lio.exe.");
                                return S_OK;
                            }

                            // Compatible with older WebView2 headers:
                            // no SetVirtualHostNameToFolderMapping, no ICoreWebView2_3, no WIL.
                            webview_->Navigate(fileUriFromPath(htmlFile).c_str());
                            return S_OK;
                        }).Get());

                return S_OK;
            }).Get());
}

void MainComponent::resizeWebView()
{
    if (!controller_)
        return;

    RECT bounds{};
    GetClientRect(hwnd_, &bounds);
    controller_->put_Bounds(bounds);
}

std::wstring MainComponent::executableDirectory() const
{
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path().wstring();
}

std::wstring MainComponent::htmlPath() const
{
    return executableDirectory() + L"\\assets\\LIO.html";
}

std::wstring MainComponent::fileUriFromPath(const std::wstring& path) const
{
    std::wstring normalized = path;
    for (wchar_t& ch : normalized)
    {
        if (ch == L'\\')
            ch = L'/';
    }

    std::wstringstream uri;
    uri << L"file:///";

    for (wchar_t ch : normalized)
    {
        if ((ch >= L'a' && ch <= L'z') ||
            (ch >= L'A' && ch <= L'Z') ||
            (ch >= L'0' && ch <= L'9') ||
            ch == L'/' || ch == L':' || ch == L'-' || ch == L'_' || ch == L'.')
        {
            uri << ch;
        }
        else
        {
            uri << L'%' << std::uppercase << std::hex << std::setw(2) << std::setfill(L'0')
                << static_cast<int>(static_cast<unsigned char>(ch))
                << std::nouppercase << std::dec;
        }
    }

    return uri.str();
}

void MainComponent::showError(const std::wstring& message) const
{
    MessageBoxW(hwnd_, message.c_str(), L"LIO", MB_ICONERROR | MB_OK);
}

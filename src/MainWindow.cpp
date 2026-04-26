#include "MainWindow.h"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <wrl.h>

using Microsoft::WRL::Callback;

namespace
{
    constexpr int         kIconId      = 101;
    constexpr int         kWindowW     = 1400;
    constexpr int         kWindowH     = 960;
    constexpr const wchar_t* kClassName = L"LIO_MainWindow";
    constexpr const wchar_t* kTitle     = L"LIO — Latent Interaction Observer";
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

MainWindow::MainWindow(HINSTANCE instance, int showCommand)
    : instance_(instance), showCommand_(showCommand)
{}

MainWindow::~MainWindow()
{
    webview_.Reset();
    controller_.Reset();

    if (comInitialized_)
        CoUninitialize();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

int MainWindow::run()
{
    if (!createWindow())
        return 1;

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}

// ---------------------------------------------------------------------------
// Window creation
// ---------------------------------------------------------------------------

bool MainWindow::createWindow()
{
    WNDCLASSW wc{};
    wc.lpfnWndProc   = MainWindow::staticWndProc;
    wc.hInstance     = instance_;
    wc.lpszClassName = kClassName;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon         = LoadIconW(instance_, MAKEINTRESOURCEW(kIconId));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    hwnd_ = CreateWindowExW(
        0, kClassName, kTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        kWindowW, kWindowH,
        nullptr, nullptr, instance_, this);

    if (!hwnd_)
        return false;

    ShowWindow(hwnd_, showCommand_);
    UpdateWindow(hwnd_);
    return true;
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------

LRESULT CALLBACK MainWindow::staticWndProc(HWND hwnd, UINT msg,
                                            WPARAM wp, LPARAM lp)
{
    MainWindow* self = nullptr;

    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    else
    {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    return self ? self->wndProc(msg, wp, lp)
                : DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT MainWindow::wndProc(UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
        initWebView();
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
        return DefWindowProcW(hwnd_, msg, wp, lp);
    }
}

// ---------------------------------------------------------------------------
// WebView2 initialisation
// ---------------------------------------------------------------------------

void MainWindow::initWebView()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (SUCCEEDED(hr))
        comInitialized_ = true;
    else if (hr != RPC_E_CHANGED_MODE)
    {
        showError(L"COM initialisation failed.");
        return;
    }

    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT envHr, ICoreWebView2Environment* env) -> HRESULT
            {
                if (FAILED(envHr) || !env)
                {
                    showError(L"Could not create WebView2 environment.\n\n"
                              L"Install the Microsoft Edge WebView2 Runtime if needed.");
                    return S_OK;
                }

                env->CreateCoreWebView2Controller(
                    hwnd_,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT ctrlHr, ICoreWebView2Controller* ctrl) -> HRESULT
                        {
                            if (FAILED(ctrlHr) || !ctrl)
                            {
                                showError(L"Could not create WebView2 controller.");
                                return S_OK;
                            }

                            controller_ = ctrl;
                            controller_->get_CoreWebView2(&webview_);
                            resizeWebView();

                            // Apply settings.
                            Microsoft::WRL::ComPtr<ICoreWebView2Settings> settings;
                            if (SUCCEEDED(webview_->get_Settings(&settings)) && settings)
                            {
                                settings->put_IsScriptEnabled(TRUE);
                                settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                                settings->put_IsWebMessageEnabled(TRUE);
                                settings->put_AreDevToolsEnabled(TRUE);
                                settings->put_AreDefaultContextMenusEnabled(TRUE);
                            }

                            // Navigate to the bundled HTML file.
                            const std::wstring html = htmlPath();

                            if (!std::filesystem::exists(html))
                            {
                                showError(L"Cannot find assets\\LIO.html next to lio.exe.");
                                return S_OK;
                            }

                            webview_->Navigate(toFileUri(html).c_str());
                            return S_OK;

                        }).Get());

                return S_OK;

            }).Get());
}

void MainWindow::resizeWebView()
{
    if (!controller_)
        return;

    RECT bounds{};
    GetClientRect(hwnd_, &bounds);
    controller_->put_Bounds(bounds);
}

// ---------------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------------

std::wstring MainWindow::exeDir() const
{
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path().wstring();
}

std::wstring MainWindow::htmlPath() const
{
    return exeDir() + L"\\assets\\LIO.html";
}

// Percent-encode a local file path into a file:/// URI.
// Compatible with older WebView2 headers (no SetVirtualHostNameToFolderMapping).
std::wstring MainWindow::toFileUri(const std::wstring& path) const
{
    std::wstringstream uri;
    uri << L"file:///";

    for (wchar_t ch : path)
    {
        if (ch == L'\\')
        {
            uri << L'/';
        }
        else if ((ch >= L'a' && ch <= L'z') ||
                 (ch >= L'A' && ch <= L'Z') ||
                 (ch >= L'0' && ch <= L'9') ||
                 ch == L'/' || ch == L':' ||
                 ch == L'-' || ch == L'_' || ch == L'.')
        {
            uri << ch;
        }
        else
        {
            // Percent-encode any other character.
            uri << L'%'
                << std::uppercase << std::hex
                << std::setw(2) << std::setfill(L'0')
                << static_cast<unsigned>(static_cast<unsigned char>(ch))
                << std::nouppercase << std::dec;
        }
    }

    return uri.str();
}

// ---------------------------------------------------------------------------
// Error reporting
// ---------------------------------------------------------------------------

void MainWindow::showError(const std::wstring& msg) const
{
    MessageBoxW(hwnd_, msg.c_str(), L"LIO", MB_ICONERROR | MB_OK);
}

#include "MainWindow.h"

#include <commdlg.h>        // GetOpenFileName / GetSaveFileName
#include <cwctype>          // towlower
#include <filesystem>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <wrl.h>

#pragma comment(lib, "Comdlg32.lib")

using Microsoft::WRL::Callback;

namespace
{
    constexpr const wchar_t* kClassName = L"LIO_MainWindow_v02";
    constexpr const wchar_t* kTitle     = L"LIO \u2014 Lab Interaction Observer";
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
    // Make the app DPI-aware so the WebView2 renders crisply on
    // high-resolution (4K/HiDPI) monitors.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

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
        kInitW, kInitH,
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
        self     = static_cast<MainWindow*>(cs->lpCreateParams);
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

    // Enforce a minimum window size so the UI never gets crushed.
    case WM_GETMINMAXINFO:
    {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
        mmi->ptMinTrackSize = { kMinW, kMinH };
        return 0;
    }

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

                            // ── WebView2 settings ─────────────────────────
                            Microsoft::WRL::ComPtr<ICoreWebView2Settings> settings;
                            if (SUCCEEDED(webview_->get_Settings(&settings)) && settings)
                            {
                                settings->put_IsScriptEnabled(TRUE);
                                settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                                settings->put_IsWebMessageEnabled(TRUE);
                                settings->put_AreDevToolsEnabled(TRUE);
                                settings->put_AreDefaultContextMenusEnabled(TRUE);
                            }

                            // ── JS -> C++ message bridge ──────────────────
                            // HTML calls: window.chrome.webview.postMessage(...)
                            // C++ receives the JSON here and acts on it.
                            webview_->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [this](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
                                    {
                                        LPWSTR raw{};
                                        args->TryGetWebMessageAsString(&raw);
                                        if (raw)
                                        {
                                            handleWebMessage(raw);
                                            CoTaskMemFree(raw);
                                        }
                                        return S_OK;
                                    }).Get(),
                                nullptr);

                            // ── Navigate to bundled HTML ──────────────────
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
    if (!controller_) return;
    RECT rc{};
    GetClientRect(hwnd_, &rc);
    controller_->put_Bounds(rc);
}

// ---------------------------------------------------------------------------
// JS <-> C++ bridge
//
// HTML side (add to LIO.html):
//
//   // Open a video via native Windows dialog:
//   window.chrome.webview.postMessage(JSON.stringify({ type: 'openVideo' }));
//
//   // Save CSV via native Windows Save-As dialog:
//   window.chrome.webview.postMessage(JSON.stringify({ type: 'saveCSV', data: csvString }));
//
//   // Receive replies from C++:
//   window.chrome.webview.addEventListener('message', e => {
//     const msg = JSON.parse(e.data);
//     if (msg.type === 'videoPath') { /* load video from msg.path */ }
//   });
// ---------------------------------------------------------------------------

void MainWindow::handleWebMessage(const std::wstring& json)
{
    // Minimal JSON dispatch — no external parser needed for these cases.

    if (json.find(L"\"openVideo\"") != std::wstring::npos)
    {
        std::wstring path = openVideoDialog();
        if (!path.empty())
        {
            // Read the file and hand the bytes to the HTML as base64.
            // A blob: URL built from these bytes has no CORS restriction,
            // so the in-app audio waveform / spectrogram works. For very
            // large files we fall back to a plain file:/// path (video
            // still plays; only the audio view is unavailable).
            constexpr std::uintmax_t kMaxInlineBytes = 350ull * 1024 * 1024;
            std::error_code ec;
            const auto size = std::filesystem::file_size(path, ec);

            if (!ec && size > 0 && size <= kMaxInlineBytes)
            {
                std::ifstream in(path, std::ios::binary);
                std::string bytes((std::istreambuf_iterator<char>(in)),
                                    std::istreambuf_iterator<char>());
                const std::wstring b64  = base64Encode(bytes);
                const std::wstring name = std::filesystem::path(path).filename().wstring();
                const std::wstring mime = mimeForExtension(path);
                std::wstring js = L"window.__nativeReply({\"type\":\"videoBytes\","
                                  L"\"data\":\"" + b64 + L"\","
                                  L"\"mime\":\"" + mime + L"\","
                                  L"\"name\":\"" + jsEscape(name) + L"\"});";
                execScript(js);
            }
            else
            {
                // Too large (or unreadable) — fall back to a path URI.
                std::wstring uri = toFileUri(path);
                std::wstring js  = L"window.__nativeReply("
                                   L"{\"type\":\"videoPath\","
                                   L"\"path\":\"" + jsEscape(uri) + L"\"});";
                execScript(js);
            }
        }
    }
    else if (json.find(L"\"saveCSV\"") != std::wstring::npos)
    {
        // Extract the "data" field (everything between first and last quote pair).
        auto dataStart = json.find(L"\"data\"");
        if (dataStart == std::wstring::npos) return;

        dataStart = json.find(L'\"', dataStart + 6);   // opening quote of value
        if (dataStart == std::wstring::npos) return;
        ++dataStart;

        auto dataEnd = json.rfind(L'\"');
        if (dataEnd == std::wstring::npos || dataEnd <= dataStart) return;

        std::wstring csvData = json.substr(dataStart, dataEnd - dataStart);

        // Unescape \n and \t that JS JSON.stringify produces.
        std::wstring csv;
        for (size_t i = 0; i < csvData.size(); ++i)
        {
            if (csvData[i] == L'\\' && i + 1 < csvData.size())
            {
                wchar_t next = csvData[i + 1];
                if      (next == L'n')  { csv += L'\n'; ++i; }
                else if (next == L't')  { csv += L'\t'; ++i; }
                else if (next == L'\\') { csv += L'\\'; ++i; }
                else                    { csv += csvData[i]; }
            }
            else csv += csvData[i];
        }

        std::wstring savePath = saveCsvDialog();
        if (savePath.empty()) return;

        // Write UTF-8 BOM + CSV so Excel opens it correctly.
        std::ofstream f(savePath, std::ios::binary);
        if (!f) { showError(L"Could not write the CSV file."); return; }
        f << "\xEF\xBB\xBF";   // UTF-8 BOM
        for (wchar_t ch : csv)
            f << static_cast<char>(ch);   // ASCII-safe for standard CSV

        execScript(L"window.__nativeReply({\"type\":\"csvSaved\"});");
    }
}

void MainWindow::execScript(const std::wstring& js)
{
    if (webview_)
        webview_->ExecuteScript(js.c_str(), nullptr);
}

// ---------------------------------------------------------------------------
// Native dialogs
// ---------------------------------------------------------------------------

std::wstring MainWindow::openVideoDialog()
{
    wchar_t buf[MAX_PATH]{};

    OPENFILENAMEW ofn{};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = hwnd_;
    ofn.lpstrFilter  = L"Video files\0*.mp4;*.avi;*.mov;*.mkv;*.wmv;*.webm\0"
                       L"All files\0*.*\0";
    ofn.lpstrFile    = buf;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrTitle   = L"Open video file";
    ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    return GetOpenFileNameW(&ofn) ? buf : L"";
}

std::wstring MainWindow::saveCsvDialog()
{
    wchar_t buf[MAX_PATH] = L"annotations.csv";

    OPENFILENAMEW sfn{};
    sfn.lStructSize  = sizeof(sfn);
    sfn.hwndOwner    = hwnd_;
    sfn.lpstrFilter  = L"CSV files\0*.csv\0All files\0*.*\0";
    sfn.lpstrFile    = buf;
    sfn.nMaxFile     = MAX_PATH;
    sfn.lpstrTitle   = L"Save annotations as CSV";
    sfn.lpstrDefExt  = L"csv";
    sfn.Flags        = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    return GetSaveFileNameW(&sfn) ? buf : L"";
}

// ---------------------------------------------------------------------------
// Path / URI helpers
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

std::wstring MainWindow::toFileUri(const std::wstring& path) const
{
    std::wstringstream uri;
    uri << L"file:///";

    for (wchar_t ch : path)
    {
        if (ch == L'\\')
            uri << L'/';
        else if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') ||
                 (ch >= L'0' && ch <= L'9') ||
                 ch == L'/' || ch == L':' || ch == L'-' || ch == L'_' || ch == L'.')
            uri << ch;
        else
            uri << L'%' << std::uppercase << std::hex
                << std::setw(2) << std::setfill(L'0')
                << static_cast<unsigned>(static_cast<unsigned char>(ch))
                << std::nouppercase << std::dec;
    }
    return uri.str();
}

std::wstring MainWindow::jsEscape(const std::wstring& s) const
{
    std::wstring out;
    out.reserve(s.size());
    for (wchar_t ch : s)
    {
        if      (ch == L'\\') out += L"\\\\";
        else if (ch == L'"')  out += L"\\\"";
        else if (ch == L'\n') out += L"\\n";
        else if (ch == L'\r') out += L"\\r";
        else                  out += ch;
    }
    return out;
}

// Standard base64 encoding of a raw byte string. The result is ASCII,
// so it is widened to wchar_t for injection into the WebView script.
std::wstring MainWindow::base64Encode(const std::string& bytes) const
{
    static const char* tbl =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::wstring out;
    out.reserve(((bytes.size() + 2) / 3) * 4);

    size_t i = 0;
    const size_t n = bytes.size();
    while (i + 2 < n)
    {
        const unsigned a = static_cast<unsigned char>(bytes[i]);
        const unsigned b = static_cast<unsigned char>(bytes[i + 1]);
        const unsigned c = static_cast<unsigned char>(bytes[i + 2]);
        const unsigned triple = (a << 16) | (b << 8) | c;
        out += static_cast<wchar_t>(tbl[(triple >> 18) & 0x3F]);
        out += static_cast<wchar_t>(tbl[(triple >> 12) & 0x3F]);
        out += static_cast<wchar_t>(tbl[(triple >>  6) & 0x3F]);
        out += static_cast<wchar_t>(tbl[ triple        & 0x3F]);
        i += 3;
    }
    if (i < n)
    {
        const unsigned a = static_cast<unsigned char>(bytes[i]);
        const unsigned b = (i + 1 < n) ? static_cast<unsigned char>(bytes[i + 1]) : 0;
        const unsigned triple = (a << 16) | (b << 8);
        out += static_cast<wchar_t>(tbl[(triple >> 18) & 0x3F]);
        out += static_cast<wchar_t>(tbl[(triple >> 12) & 0x3F]);
        out += (i + 1 < n) ? static_cast<wchar_t>(tbl[(triple >> 6) & 0x3F]) : L'=';
        out += L'=';
    }
    return out;
}

std::wstring MainWindow::mimeForExtension(const std::wstring& path) const
{
    std::wstring ext = std::filesystem::path(path).extension().wstring();
    for (wchar_t& c : ext) c = towlower(c);
    if (ext == L".webm") return L"video/webm";
    if (ext == L".ogg" || ext == L".ogv") return L"video/ogg";
    if (ext == L".avi")  return L"video/x-msvideo";
    if (ext == L".mov")  return L"video/quicktime";
    if (ext == L".mkv")  return L"video/x-matroska";
    return L"video/mp4";   // mp4 / m4v / default
}

void MainWindow::showError(const std::wstring& msg) const
{
    MessageBoxW(hwnd_, msg.c_str(), L"LIO", MB_ICONERROR | MB_OK);
}

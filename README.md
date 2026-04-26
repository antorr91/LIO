# LIO — Latent Interaction Observer

A lightweight desktop tool for annotating animal behaviour from video.  
Built with **C++ 20**, **Win32**, and **WebView2** — no Electron, no JUCE.

---

## Features

- Load any local video and play it at variable speed (0.25× → 2×)
- Click directly on the arena to register behavioural events with precise timestamps
- Define custom zones (ROI) and track zone entries automatically
- Real-time distance tracking with a calibrated px → m scale
- Recent Annotations panel with one-click delete for accidental clicks
- Volume control with SVG speaker icon (mute / low / full)
- Export all annotations to CSV

---

## Requirements

| Tool | Version |
|------|---------|
| Windows | 10 / 11 (x64) |
| Visual Studio Build Tools | 2022 — *Desktop development with C++* |
| CMake | ≥ 3.21 |
| vcpkg | latest |
| Microsoft Edge WebView2 Runtime | pre-installed on Win10 21H2+ |

---

## Build

### 1. Install vcpkg (first time only)
```powershell
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
C:\dev\vcpkg\bootstrap-vcpkg.bat
C:\dev\vcpkg\vcpkg install webview2:x64-windows
```

### 2. Clone the repo
```powershell
git clone https://github.com/antorr91/LIO.git
cd LIO
```

### 3. Configure and build
```powershell
cmake -S . -B build `
  -DCMAKE_TOOLCHAIN_FILE=C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows

cmake --build build --config Release
```

### 4. Run
```powershell
.\build\Release\lio.exe
```

The executable looks for `assets\LIO.html` in the same folder — the CMake
post-build step copies it there automatically.

---

## Project structure

```
LIO/
├── CMakeLists.txt
├── vcpkg.json
├── .gitignore
├── assets/
│   ├── LIO.html        ← full app UI (single self-contained file)
│   └── LIO.ico         ← window & taskbar icon
└── src/
    ├── main.cpp
    ├── MainWindow.h
    ├── MainWindow.cpp
    └── lio_resource.rc.in
```

---

## How it works

The C++ layer creates a native Win32 window and hosts a **WebView2** control
that fills it entirely. All application logic — video playback, annotation,
trajectory drawing, CSV export — lives in `assets/LIO.html` as a single
self-contained HTML/CSS/JS file. This keeps the native code minimal and the
UI fully editable without recompiling.

---

## License

MIT

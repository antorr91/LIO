# LIO v01 — WebView2/no-JUCE build with updated HTML UI and logo

This package uses the latest `LIO.html` interface and the external `LIO_logo.ico`.

It does **not** use JUCE.

## What changed in this package

- Uses the latest uploaded `LIO.html` as the app UI.
- Uses `assets/LIO_logo.ico` in the HTML header.
- Uses the same icon as the Windows `.exe` icon.
- Keeps the WebView2 implementation that worked on your machine.
- Avoids `SetVirtualHostNameToFolderMapping`, so it remains compatible with your current WebView2 headers.
- Keeps `main.cpp`, `MainComponent.h`, and `MainComponent.cpp`.

## Folder structure

```text
LIO_v01_WebView2_updated_UI_logo/
├── CMakeLists.txt
├── vcpkg.json
├── README_BUILD.md
├── assets/
│   ├── LIO.html
│   ├── LIO.ico
│   └── LIO_logo.ico
└── src/
    ├── main.cpp
    ├── MainComponent.h
    ├── MainComponent.cpp
    └── lio_resource.rc.in
```

## Build from PowerShell

```powershell
cd "C:\Users\anton\Downloads\LIO_v01_WebView2_updated_UI_logo"

Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows

cmake --build build --config Release
```

## Run

```powershell
.\build\Release\lio.exe
```

## If WebView2 is not installed

```powershell
C:\dev\vcpkg\vcpkg install webview2:x64-windows
```

Then repeat the two CMake commands.

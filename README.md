<p align="center">
  <img src="assets/LIO_logo.png" alt="LIO logo" width="120"/>
</p>

<h1 align="center">LIO — Lab Interaction Observer</h1>

<p align="center">
  A desktop tool for annotating animal behaviour directly on video.<br/>
  Designed for ethology researchers. No internet required. Just run the exe.
</p>

<p align="center">
  <img src="assets/screenshot_lio_v01.png" alt="LIO screenshot" width="860"/>
</p>

## What it does

LIO lets you load a local video and annotate behavioural events frame by frame by clicking directly on the animal inside the arena.

- **Click on the animal** → registers the event with the exact video timestamp
- **Define zones** (ROI) on the arena → zone entries are detected automatically
- **Track distance** in real metres with a simple px → m calibration
- **Recent Annotations** panel → delete accidental clicks with one click
- **Export** to CSV (summary + event log) or save the full session as JSON to resume later

## How it works

```
lio.exe
└── assets/
    └── LIO.html   ← the entire app (video player, annotation engine, charts, export)
```

The native C++ layer opens a window and hosts a WebView2 control inside it.  
All logic lives in a single self-contained HTML file — no frameworks, no build step for the UI.

## Run

Download the latest release, unzip and double-click `lio.exe`.  
Requires **Windows 10 / 11** with the [WebView2 Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/) (already installed on most machines).

## Build from source

```powershell
git clone https://github.com/antorr91/LIO.git
cd LIO

cmake -S . -B build `
  -DCMAKE_TOOLCHAIN_FILE=C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows

cmake --build build --config Release
.\build\Release\lio.exe
```

Requires: **Visual Studio 2022 Build Tools** · **CMake ≥ 3.21** · **vcpkg** with `webview2`

<p align="center">MIT License · made by <a href="https://github.com/antorr91">antorr91</a></p>

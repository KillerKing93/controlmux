# ControlMux 🖱️⌨️

> An **ultra-lightweight, multi-person input control utility** for Windows and Linux (Arch Linux, Debian, CachyOS, SteamOS, Fedora, Ubuntu, openSUSE) inspired by [MouseMux](https://www.mousemux.com/).

Created by **Alif Nurhidayat** (`alifnurhidayatwork@gmail.com`).

ControlMux enables multiple users on a single PC to work concurrently using separate hardware mouse and keyboard pairs. Each person receives a dedicated virtual cursor overlay, custom color, and isolated input focus context—ensuring that typing on one user's keyboard never leaks into another user's active window.

---

## ⚡ Highlights

- **Author**: Alif Nurhidayat (<alifnurhidayatwork@gmail.com>)
- **Ultra-Lightweight & Fast**: Single native C++17 binary (**~191 KB** executable size, **~6.5 MB** RAM usage, **< 0.1%** CPU usage).
- **Cross-Platform Linux & Windows Support**: Native builds on **Arch Linux, Debian, CachyOS, SteamOS, Fedora, Ubuntu, and openSUSE**.
- **Zero Heavy Dependencies**: Built using native platform APIs (`Win32 Raw Input`, `GDI+` on Windows; `libevdev`, `/dev/input/`, `X11`/`Cairo` on Linux). No heavy Python, Node, or Electron runtime needed.
- **Hardware-Level Device Pairing**: Maps physical mice and keyboards by unique hardware HID instance IDs (`HID\VID_xxxx&PID_xxxx` or `/dev/input/by-id/`).
- **Multi-Cursor Overlay**: Transparent, double-buffered screen overlay with colored pointer arrows, click ripple animations, and person name badges (`Person 1`, `Person 2`).
- **Focus Isolation & Input Routing**:
  - **Switched Focus Mode**: Seamlessly switches active OS mouse focus when a person moves/clicks, while isolating secondary keyboards to prevent cross-person keypress pollution.
  - **Direct Target Mode**: Directs keypresses via native message injection to each person's target window handle.
- **System Tray Management**: Easily toggle control, switch routing modes, and view device profiles from the system tray menu.

---

## 🏗️ Architecture & How It Works

```
                     ┌───────────────────────────────┐
                     │ Physical Devices (Raw Input)  │
                     │  Mouse 1, Keyboard 1 (Red)    │
                     │  Mouse 2, Keyboard 2 (Blue)   │
                     └───────────────┬───────────────┘
                                     │
                                     ▼
                     ┌───────────────────────────────┐
                     │     Input Engine (WM_INPUT)   │
                     │ Identifies hDevice & Person   │
                     └───────────────┬───────────────┘
                                     │
             ┌───────────────────────┴───────────────────────┐
             ▼                                               ▼
┌─────────────────────────┐                     ┌─────────────────────────┐
│     Focus Router        │                     │   Overlay Renderer      │
│  Resolves target HWND   │                     │ Topmost transparent     │
│  Isolates & routes keys │                     │ GDI+ double-buffer canvas│
└─────────────────────────┘                     └─────────────────────────┘
```

1. **Device Manager (`src/device_manager.hpp / .cpp / _linux.cpp`)**: Enumerates HID devices (`GetRawInputDeviceList` on Windows, `libevdev` / `/dev/input/` on Linux) and maps physical device handles to `PersonState` profiles.
2. **Input Engine (`src/input_engine.hpp / .cpp`)**: Intercepts raw input events before OS aggregation.
3. **Overlay Renderer (`src/overlay_renderer.hpp / .cpp`)**: Renders virtual pointers and user badges on a click-through layered window.
4. **Focus Router (`src/focus_router.hpp / .cpp`)**: Computes target window handles under each person's cursor and handles keystroke isolation/routing.
5. **System Tray GUI (`src/gui_win32.hpp / .cpp`)**: Controls tray icon tooltip, popup menus, and pairing dialogs.

---

## 🚀 Building ControlMux

### Windows (MinGW-W64 GCC)
```bash
# Compile controlmux.exe
g++ -O2 -std=c++17 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -municode -mwindows -Isrc -o controlmux.exe src/main.cpp src/config.cpp src/device_manager.cpp src/overlay_renderer.cpp src/focus_router.cpp src/input_engine.cpp src/gui_win32.cpp -luser32 -lgdi32 -lgdiplus -lhid -lshell32
```

Or using Makefile:
```bash
make
```

### Linux (Arch Linux, CachyOS, SteamOS, Debian/Ubuntu, Fedora)
Ensure dependencies are installed (`libevdev`, `x11`, `cairo`):

- **Arch Linux / CachyOS / SteamOS**:
  ```bash
  sudo pacman -S gcc make cmake libevdev libx11 cairo
  make -f Makefile.linux
  ```
- **Debian / Ubuntu**:
  ```bash
  sudo apt install build-essential libevdev-dev libx11-dev libcairo2-dev
  make -f Makefile.linux
  ```

### CMake Build System (Cross-Platform)
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

---

## ⚖️ License Terms ([LICENSE.md](file:///d:/CraftThingy/controlmux/LICENSE.md))

Copyright (c) 2026 **Alif Nurhidayat** (`alifnurhidayatwork@gmail.com`).

- **Personal & Non-Commercial Use**: Completely **FREE** to use, modify, and study for personal, research, or educational purposes.
- **Share-Alike Requirement**: Any modifications, forks, or derivative works must be publicly shared open-source under the exact same license terms.
- **Corporate & Commercial Royalty**: Commercial entities, corporations, and revenue-generating organizations **must pay a commercial royalty license** to Alif Nurhidayat (`alifnurhidayatwork@gmail.com`) to deploy, integrate, or use ControlMux in business environments.

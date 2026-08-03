<p align="center">
  <img src="assets/logo.svg" alt="ControlMux Application Logo" width="480">
</p>

<h1 align="center">ControlMux 🖱️⌨️</h1>

<p align="center">
  <b>Ultra-Lightweight Multi-Person Input Multiplexing Engine for Windows & Linux</b>
  <br>
  <i>Empower up to 16 concurrent users to operate on a single PC with dedicated mice, keyboards, virtual cursors, and focus isolation.</i>
</p>

<p align="center">
  Created by <b>Alif Nurhidayat</b> (<a href="mailto:alifnurhidayatwork@gmail.com">alifnurhidayatwork@gmail.com</a>)
</p>

---

## ⚡ Key Highlights

- **Author**: Alif Nurhidayat (<alifnurhidayatwork@gmail.com>)
- **Official Application Logo**: [`assets/logo.svg`](assets/logo.svg) — Vector 1:1 square master logo designed for app launchers, system tray, and taskbar icons.
- **Up to 16 Persons Multi-Seat Support**: Supports scaling seamlessly from 1 to **16 concurrent persons** with dedicated cursor colors, focus routing, and custom name profiles.
- **1-Click Interactive Device Pairing**: Pair physical mice and keyboards effortlessly by clicking **`[ 🎯 Mouse ]`** or **`[ 🎯 KB ]`** and moving/clicking the device. Single click toggles to **`[ 🔄 Reset ]`** to unassign back to auto.
- **Multi-Monitor & DPI V2 Precision Engine**:
  - Native **Per-Monitor DPI Awareness V2** ensures 1:1 physical pixel matching across multi-monitor setups with mixed resolutions (1080p, 1440p, 4K, ultrawides) and scaling factors (100%, 125%, 150%, 200%).
  - **`MonitorFromPoint` Rectangle Clamping** prevents cursor drift and keeps secondary cursors safely inside physical monitor bounds without getting lost in multi-display "dead zones".
  - **Continuous `HWND_TOPMOST` Z-Order**: Overlay stays permanently pinned on top of all windows and applications.
- **Ultra-Lightweight & Sub-Millisecond Speed**: Single native C++17 binary (**~200 KB** executable size, **~7.5 MB** RAM usage, **< 0.1%** CPU usage) with cached HID hardware lookups and instant 0-lag `Add Person` / `Remove Person` execution.
- **Cross-Platform Linux & Windows Support**: Native builds on **Windows 10/11, Arch Linux, CachyOS, SteamOS, Debian, Ubuntu, Fedora, and openSUSE**.
- **Zero Heavy Dependencies**: Built using native platform APIs (`Win32 Raw Input`, `GDI+` on Windows; `libevdev`, `/dev/input/`, `X11`/`Cairo` on Linux). No Python, Node, or Electron runtimes required.
- **Focus Isolation & Input Routing**:
  - **Switched Focus Mode**: Switches active OS mouse focus when a person moves/clicks, while isolating secondary keyboards to prevent cross-person keypress pollution.
  - **Direct Target Mode**: Directs keypresses via native message injection to each person's target window handle.
- **Control Center Panel & Desktop Integration**: Dark-mode floating Control Center panel on Windows and standard `.desktop` launcher integration on Linux (`assets/controlmux.desktop`).

---

## 🎯 Control Center & Interactive Device Pairing

```
┌─────────────────────────────────────────────────────────────┐
│ ControlMux — Control Center                            [✕]  │
├─────────────────────────────────────────────────────────────┤
│ ⬤ ACTIVE     ⇄ Switched Focus                Profiles: 3/16 │
├─────────────────────────────────────────────────────────────┤
│ #  PERSON NAME        MOUSE PAIR      KEYBOARD PAIR   RESET │
│ 1  Person 1           ✔ Mouse          ✔ KB            🔄 Reset│
│ 2  Person 2           🎯 Mouse         🎯 KB           🔄 Reset│
│ 3  Person 3           🎯 Mouse         🎯 KB           🔄 Reset│
├─────────────────────────────────────────────────────────────┤
│  [ ＋ Add Person ]   [ － Remove Last ]   [ ✕ Exit ]        │
└─────────────────────────────────────────────────────────────┘
```

- **`[ 🎯 Mouse ]`**: Click to enter interactive pairing mode. Move or click the target mouse to lock its Hardware ID (`HID\VID_xxxx&PID_xxxx`).
- **`[ 🎯 KB ]`**: Click to enter interactive pairing mode. Press any key on the target keyboard to lock it.
- **`[ 🔄 Reset ]`**: 1-click unbind toggle to return devices to automatic HID enumeration.
- **Scrollable Person List**: Mouse wheel and `▲` / `▼` navigation supporting up to 16 profiles.

---

## 🎨 Layered Vector Logo System (Photoshop / Illustrator / Figma Ready)

The ControlMux logo is built as a **1:1 square multi-layer SVG vector graphics system** so you can freely edit, swap, or tweak any element:

- 📄 **Combined Layered Master Vector**: [`assets/logo.svg`](assets/logo.svg) (contains labeled `<g id="layer-01-background">`, `<g id="layer-03-cyan-cursor">` layer groups)
- 📂 **Standalone Layer SVGs**: [`assets/logo_layers/`](assets/logo_layers/)
  - `01_background.svg` (Dark gradient background, corner bracket edge accents, honeycomb tech mesh & particle grid)
  - `02_circuit_core.svg` (Quantum multiplexer core reactor & bottom-attached mouse cables)
  - `03_cyan_cursor.svg` (Person 1 cyan neon 3D pointer, click ripple rings, mini keyboard icon & glassmorphic badge)
  - `04_magenta_cursor.svg` (Person 2 magenta neon 3D pointer, click ripple rings, mini keyboard icon & glassmorphic badge)
  - `05_typography.svg` (ControlMux title typography, author credit & `v1.0.0 NATIVE C++17` build badge)

---

## 🏗️ Architecture & How It Works

```
                     ┌───────────────────────────────┐
                     │ Physical Devices (Raw Input)  │
                     │  Mice & Keyboards (1 to 16)   │
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
│  Resolves target HWND   │                     │ Topmost DPI V2 Canvas   │
│  Isolates & routes keys │                     │ Multi-Monitor 1:1 Pixel │
└─────────────────────────┘                     └─────────────────────────┘
```

1. **Device Manager (`src/device_manager.hpp / .cpp / _linux.cpp`)**: Enumerates HID devices (`GetRawInputDeviceList` on Windows, `libevdev` / `/dev/input/` on Linux), caches hardware IDs, and maps physical device handles to `PersonState` profiles.
2. **Input Engine (`src/input_engine.hpp / .cpp`)**: Intercepts raw input events before OS aggregation.
3. **Overlay Renderer (`src/overlay_renderer.hpp / .cpp`)**: Renders virtual pointers and user badges on a click-through layered window with DPI V2 awareness and per-monitor bounds.
4. **Focus Router (`src/focus_router.hpp / .cpp`)**: Computes target window handles under each person's cursor and handles keystroke isolation/routing.
5. **Control Center GUI (`src/gui_win32.hpp / .cpp`)**: Floating Win32 Control Center window supporting 1–16 persons, 1-click pairing, mode switching, and smooth scrolling.

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

## ⚖️ License Terms ([LICENSE.md](LICENSE.md))

Copyright (c) 2026 **Alif Nurhidayat** (`alifnurhidayatwork@gmail.com`).

- **Personal, Educational, & Non-Commercial Use**: 100% **FREE** for personal, research, or educational purposes under share-alike terms.
- **Commercial & Corporate Use**: Requires a commercial royalty license (**$29.00 USD per workstation seat** or **7.5% gross royalty fee** for embedded/bundled products). Contact Alif Nurhidayat (`alifnurhidayatwork@gmail.com`) for corporate licenses.

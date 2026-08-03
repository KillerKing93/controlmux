# ControlMux 🖱️⌨️

> An **ultra-lightweight, multi-person input control utility** for Windows and Linux inspired by [MouseMux](https://www.mousemux.com/).

ControlMux enables multiple users on a single PC to work concurrently using separate hardware mouse and keyboard pairs. Each person receives a dedicated virtual cursor overlay, custom color, and isolated input focus context—ensuring that typing on one user's keyboard never leaks into another user's active window.

---

## ⚡ Highlights

- **Ultra-Lightweight & Fast**: Single native C++17 binary (**~191 KB** executable size, **~6.5 MB** RAM usage, **< 0.1%** CPU usage).
- **Zero Heavy Dependencies**: Built using native platform APIs (`Win32 Raw Input`, `GDI+`, `Low-Level Hooks` on Windows; `evdev` / `X11` / `uinput` on Linux). No Python, Node, or Electron runtime needed.
- **Hardware-Level Device Pairing**: Maps physical mice and keyboards by unique hardware HID instance IDs (`HID\VID_xxxx&PID_xxxx`).
- **Multi-Cursor Overlay**: Transparent, double-buffered screen overlay with colored pointer arrows, click ripple animations, and person name badges (`Person 1`, `Person 2`).
- **Focus Isolation & Input Routing**:
  - **Switched Focus Mode**: Seamlessly switches active OS mouse focus when a person moves/clicks, while isolating secondary keyboards to prevent cross-person keypress pollution.
  - **Direct Target Mode**: Directs keypresses via Win32 message injection (`PostMessageW` / `WM_KEYDOWN` / `WM_CHAR`) to each person's target window handle.
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

1. **Device Manager (`src/device_manager.hpp / .cpp`)**: Enumerates HID devices (`GetRawInputDeviceList`, `GetRawInputDeviceInfoW`) and maps physical device handles to `PersonState` profiles.
2. **Input Engine (`src/input_engine.hpp / .cpp`)**: Registers for Raw Input (`RIDEV_INPUTSINK`) to intercept input events before OS aggregation.
3. **Overlay Renderer (`src/overlay_renderer.hpp / .cpp`)**: Renders virtual pointers and user badges on a click-through layered window (`WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT`).
4. **Focus Router (`src/focus_router.hpp / .cpp`)**: Computes target window handles under each person's cursor and handles keystroke isolation/routing.
5. **System Tray GUI (`src/gui_win32.hpp / .cpp`)**: Controls tray icon tooltip, popup menus, and pairing dialogs.

---

## 🚀 Building ControlMux

### Requirements
- **Windows**: MinGW-W64 GCC 13+ (or MSVC) with GDI+ and HID libraries.
- **Linux**: GCC/Clang with `libevdev` and `X11` development headers.

### Windows (MinGW-W64 GCC)
```bash
# Direct g++ compilation
g++ -O2 -std=c++17 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -municode -mwindows -Isrc -o controlmux.exe src/main.cpp src/config.cpp src/device_manager.cpp src/overlay_renderer.cpp src/focus_router.cpp src/input_engine.cpp src/gui_win32.cpp -luser32 -lgdi32 -lgdiplus -lhid -lshell32
```

Or using Makefile:
```bash
make
```

### CMake Build System (Cross-Platform)
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

---

## 🔧 Usage & Device Setup

1. Run `controlmux.exe`. ControlMux will launch into the Windows **System Tray**.
2. Right-click the **ControlMux Tray Icon** to open the context menu:
   - **✔ ControlMux Enabled / Paused**: Toggle global input multiplexing.
   - **Mode: Switched Focus**: Active person holds OS focus; secondary keyboards are isolated.
   - **Mode: Direct Window Target**: Directs keypresses to windows under each person's virtual cursor.
   - **Pair Devices Wizard...**: View current hardware pairings.
3. Settings are automatically saved to `controlmux_config.ini`.

---

## 📂 Source Code Directory Structure

```
controlmux/
├── Makefile                          # MinGW GCC build script
├── CMakeLists.txt                    # Cross-platform CMake setup
├── README.md                         # Project documentation
├── src/
│   ├── main.cpp                      # WinMain entry point & Win32 event loop
│   ├── config.hpp / .cpp             # Configuration settings & ini parser
│   ├── device_manager.hpp / .cpp     # HID hardware discovery & PersonState
│   ├── overlay_renderer.hpp / .cpp   # GDI+ transparent overlay renderer
│   ├── focus_router.hpp / .cpp       # Target HWND resolution & focus isolation
│   ├── input_engine.hpp / .cpp       # Win32 Raw Input (WM_INPUT) engine
│   └── gui_win32.hpp / .cpp          # System Tray icon & context menu UI
```

---

## 📜 License

MIT License - feel free to modify and extend ControlMux!

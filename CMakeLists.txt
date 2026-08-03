cmake_minimum_required(VERSION 3.10)
project(ControlMux CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    set(SOURCES
        src/main.cpp
        src/config.cpp
        src/device_manager.cpp
        src/overlay_renderer.cpp
        src/focus_router.cpp
        src/input_engine.cpp
        src/gui_win32.cpp
    )

    add_executable(controlmux WIN32 ${SOURCES})
    target_link_libraries(controlmux user32 gdi32 gdiplus hid shell32)
else()
    # Linux build configuration using libevdev / X11
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(X11 REQUIRED x11)
    pkg_check_modules(EVDEV REQUIRED libevdev)

    set(SOURCES
        src/main.cpp
        src/config.cpp
        src/device_manager.cpp
        src/overlay_renderer.cpp
        src/focus_router.cpp
        src/input_engine.cpp
    )

    add_executable(controlmux ${SOURCES})
    target_include_directories(controlmux PRIVATE ${X11_INCLUDE_DIRS} ${EVDEV_INCLUDE_DIRS})
    target_link_libraries(controlmux ${X11_LIBRARIES} ${EVDEV_LIBRARIES} pthread)
endif()

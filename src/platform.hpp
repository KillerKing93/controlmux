/**
 * @file platform.hpp
 * @brief Cross-platform abstraction header for Windows and Linux systems.
 */

#ifndef CONTROLMUX_PLATFORM_HPP
#define CONTROLMUX_PLATFORM_HPP

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <X11/Xlib.h>
  #include <X11/Xutil.h>
  #include <libevdev/libevdev.h>
  
  // Cross-platform type aliases for Linux compilation
  typedef void* HANDLE;
  typedef unsigned long DWORD;
  typedef unsigned short USHORT;
  typedef unsigned long Window;
  typedef Window HWND;
#endif

#endif // CONTROLMUX_PLATFORM_HPP

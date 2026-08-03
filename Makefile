CXX = g++
WINDRES = windres
CXXFLAGS = -O2 -std=c++17 -Wall -Wextra -Wno-unused-parameter -municode -mwindows
INCLUDES = -Isrc
LIBS = -luser32 -lgdi32 -lgdiplus -lhid -lshell32

TARGET = controlmux.exe

SRCS = src/main.cpp \
       src/config.cpp \
       src/device_manager.cpp \
       src/overlay_renderer.cpp \
       src/focus_router.cpp \
       src/input_engine.cpp \
       src/gui_win32.cpp

OBJS = $(SRCS:.cpp=.o) src/controlmux_rc.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)
	@echo Build complete with embedded logo icon: $(TARGET)

src/controlmux_rc.o: src/controlmux.rc assets/app_icon.ico
	$(WINDRES) src/controlmux.rc -O coff -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)

.PHONY: all clean

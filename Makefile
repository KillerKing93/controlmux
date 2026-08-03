CXX = g++
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

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)
	@echo Build complete: $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)

.PHONY: all clean

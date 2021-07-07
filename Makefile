#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need SDL2 (http://www.libsdl.org):
# Linux:
#   apt-get install libsdl2-dev
# Mac OS X:
#   brew install sdl2
# MSYS2:
#   pacman -S mingw-w64-i686-SDL2
#

# pick one
CXX = g++
#CXX = clang++

EXE = reinette
SOURCES = main.cpp puce65c02.cpp mmu.cpp video.cpp disk.cpp speaker.cpp paddles.cpp gui.cpp

IMGUI_DIR = lib/imgui-1.82
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl2.cpp

ImGuiColorTextEdit_DIR = lib/ImGuiColorTextEdit-master
SOURCES += $(ImGuiColorTextEdit_DIR)/TextEditor.cpp

imfilebrowser_DIR = lib/imgui-filebrowser
imgui_club_DIR = lib/imgui_club-master


OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

CXXFLAGS = -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(ImGuiColorTextEdit_DIR) -I$(imfilebrowser_DIR) -I$(imgui_club_DIR)
# CXXFLAGS += -O3
CXXFLAGS += -std=c++17 -lstdc++fs -Wall -Wformat -pedantic -Wpedantic -O3
LIBS =

WIN32-RC = reinette.rc
WIN32-RES = reinette.res


##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -lGL -ldl `sdl2-config --libs`

	CXXFLAGS += `sdl2-config --cflags`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`
	LIBS += -L/usr/local/lib -L/opt/local/lib

	CXXFLAGS += `sdl2-config --cflags`
	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(OS), Windows_NT)
	ECHO_MESSAGE = "MinGW"
	# LIBS += -lgdi32 -lopengl32 -limm32 `pkg-config --static --libs sdl2`
	LIBS += -lgdi32 -lopengl32 -limm32 -lmingw32 --static  -Wl,-subsystem,windows -LC:/msys64/mingw64/lib -lmingw32 -lSDL2main -lSDL2 -mwindows -lmingw32 -ldinput8 -lshell32 -lsetupapi -ladvapi32 -luuid -lversion -loleaut32 -lole32 -limm32 -lwinmm -lgdi32 -luser32 -lm -mwindows -Wl,--no-undefined -pipe -Wl,--dynamicbase,--high-entropy-va,--nxcompat,--default-image-base-high

	# CXXFLAGS += `pkg-config --cflags sdl2 `
	CXXFLAGS += -IC:/msys64/mingw64/include/SDL2
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(ImGuiColorTextEdit_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<



all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS) $(WIN32-RES)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

$(WIN32-RES): $(WIN32-RC)
	windres -o $@ $^ -O coff

clean:
	rm -f $(EXE) $(OBJS) $(WIN32-RES)

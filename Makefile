# Vesperis — build with the Apple toolchain and Homebrew raylib.
CXX      ?= /usr/bin/clang++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Isrc -MMD -MP -pthread
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS   := $(shell pkg-config --libs raylib 2>/dev/null)
# R-418: raylib's static library when there is one, as the release builds link it (the platform layer calls GLFW's joystick functions,
# which Homebrew's shared library keeps hidden)
RAYLIB_STATIC := $(wildcard $(shell pkg-config --variable=libdir raylib 2>/dev/null)/libraylib.a)
ifneq (,$(RAYLIB_STATIC))
  RAYLIB_LIBS := $(RAYLIB_STATIC)
endif
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  RAYLIB_LIBS += -framework CoreVideo -framework IOKit -framework Cocoa -framework OpenGL
  ifneq (,$(wildcard /opt/homebrew/opt/raylib/lib))
    RAYLIB_LIBS += -Wl,-rpath,/opt/homebrew/opt/raylib/lib
  endif
endif

CORE_SRC  := $(wildcard src/core/*.cpp) $(wildcard src/galaxy/*.cpp) $(wildcard src/space/*.cpp) $(wildcard src/surface/*.cpp) $(wildcard src/game/*.cpp)
CORE_OBJ  := $(patsubst src/%.cpp,build/%.o,$(CORE_SRC))

all: vesperis vesperis_test

vesperis: $(CORE_OBJ) build/main_raylib.o build/platform_raylib.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(RAYLIB_LIBS)

vesperis_test: $(CORE_OBJ) build/main_headless.o
	$(CXX) $(CXXFLAGS) -o $@ $^

build/platform_raylib.o: src/platform_raylib.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(RAYLIB_CFLAGS) -c -o $@ $<

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf build vesperis vesperis_test

-include $(CORE_OBJ:.o=.d) build/main_raylib.d build/main_headless.d build/platform_raylib.d

.PHONY: all clean

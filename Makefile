# Vesperis — build with the Apple toolchain and Homebrew raylib. `make check` holds the tree's layout (tools/check.py).
ifeq ($(origin CXX),default)
  CXX := /usr/bin/clang++
endif
# E-02: flags come from make's command line (`make CXXFLAGS="-O0 -g"`), never from the shell's environment (Homebrew's LLVM
# exports a CPPFLAGS of its own); the include path and the dependency files stay whatever is passed
ifneq ($(origin CXXFLAGS),command line)
  CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pthread
endif
ifneq ($(origin CPPFLAGS),command line)
  CPPFLAGS :=
endif
override CPPFLAGS += -Isrc -MMD -MP
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
HARNESS_SRC := $(wildcard src/harness/*.cpp)
HARNESS_OBJ := $(patsubst src/%.cpp,build/%.o,$(HARNESS_SRC))
# E-02: the object directories, one per source directory, are all a clean removes besides the top-level objects and the
# binaries: build/ also keeps toolchains/, perf-baselines/, prove/ and the milestones' kits
OBJ_DIRS  := $(sort $(patsubst src/%/,build/%,$(dir $(CORE_SRC) $(HARNESS_SRC))))

all: vesperis vesperis_test

vesperis: $(CORE_OBJ) build/main_raylib.o build/platform_raylib.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(RAYLIB_LIBS)

vesperis_test: $(CORE_OBJ) build/main_headless.o $(HARNESS_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/platform_raylib.o: src/platform_raylib.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(RAYLIB_CFLAGS) -c -o $@ $<

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIRS) build/*.o build/*.d vesperis vesperis_test

check:
	@python3 tools/check.py

-include $(CORE_OBJ:.o=.d) $(HARNESS_OBJ:.o=.d) build/main_raylib.d build/main_headless.d build/platform_raylib.d

.PHONY: all clean check

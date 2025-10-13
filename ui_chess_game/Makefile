# Makefile - Windows-compatible version
PROJECT = tinychess
OUTDIR  = build
TARGET  = $(OUTDIR)/$(PROJECT).exe

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall
INCDIRS  = -Iinclude -Iinclude/imgui -Iinclude/SDL3 -I../src -Iinclude/SDL3_image
LIBDIRS  = -Llib
LIBS     = -lSDL3 -lSDL3_image

SRCS = $(wildcard src/*.cpp) $(wildcard imgui/*.cpp) ../src/core/position.cc ../src/core/bitboard.cc ../src/core/movegen.cc ../src/minmax/minmax.cc
OBJS = $(SRCS:.cpp=.o)

.PHONY: default all clean

default: all

all: $(OUTDIR) $(TARGET)

# create output dir if missing
$(OUTDIR):
	@if not exist $(OUTDIR) mkdir $(OUTDIR)

# link
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCDIRS) -o $@ $^ $(LIBDIRS) $(LIBS)

# Specific compilation rules for each source type
src/main.o: src/main.cpp
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $< -o $@

imgui/%.o: imgui/%.cpp
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $< -o $@

../src/core/%.o: ../src/core/%.cc
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $< -o $@

../src/minmax/%.o: ../src/minmax/%.cc
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $< -o $@

clean:
	@if exist src\*.o del /q src\*.o
	@if exist imgui\*.o del /q imgui\*.o
	@if exist ..\src\core\*.o del /q ..\src\core\*.o
	@if exist ..\src\minmax\*.o del /q ..\src\minmax\*.o
	@if exist build\test.exe del /q build\test.exe

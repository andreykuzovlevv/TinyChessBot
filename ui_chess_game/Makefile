# Makefile - minimal, works with g++ (MinGW/MSYS, Linux)
PROJECT = test
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
	@mkdir -p $(OUTDIR)

# link
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCDIRS) -o $@ $^ $(LIBDIRS) $(LIBS)

# compile .cpp -> .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $< -o $@

clean:
	-rm -f src/*.o imgui/*.o ../src/core/*.o ../src/minmax/*.o $(TARGET)

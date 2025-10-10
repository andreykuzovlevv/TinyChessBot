# Makefile - minimal, works with g++ (MinGW/MSYS, Linux)
PROJECT = test
OUTDIR  = build
TARGET  = $(OUTDIR)/$(PROJECT).exe

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall
INCDIRS  = -Iinclude -Iinclude/imgui -Iinclude/SDL3
LIBDIRS  = -Llib
LIBS     = -lSDL3

SRCS = $(wildcard src/*.cpp) $(wildcard imgui/*.cpp)
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
	-rm -f src/*.o imgui/*.o $(TARGET)

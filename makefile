CXX := g++

# pull in ROOT_CFLAGS and ROOT_LIBS (no error if missing)
-include root_flags.mk
USE_ROOT ?= 0
ROOT_CFLAGS ?=
ROOT_LIBS   ?=
ROOT_SYS_CFLAGS := $(patsubst -I%,-isystem %,$(ROOT_CFLAGS))
ROOT_OTHER_FLAGS := $(filter-out -I%,$(ROOT_CFLAGS))

# only define USE_ROOT when actually enabled
ifeq ($(USE_ROOT),1)
    ROOT_DEFINE := -DUSE_ROOT=1
else
    ROOT_DEFINE :=
endif

# Common include flags
INCLUDES := -Iinclude

# Release flags
CXXFLAGS_RELEASE := -std=c++20 -O3 -march=native $(ROOT_DEFINE) $(INCLUDES) $(ROOT_SYS_CFLAGS) $(ROOT_OTHER_FLAGS)

# Debug flags
CXXFLAGS_DEBUG := -std=c++20 -O0 -g $(ROOT_DEFINE) $(INCLUDES) $(ROOT_SYS_CFLAGS) $(ROOT_OTHER_FLAGS)

# Source lists
GCC_SRCS_CONVERT := \
	src/PhaseSpaceFileReader.cc \
	src/PhaseSpaceFileWriter.cc \
	src/utilities/formats.cc \
	src/egs/egsphspFile.cc \
	src/peneasy/penEasyphspFile.cc \
	src/iaea/IAEAHeader.cc \
	src/iaea/IAEAphspFile.cc \
	src/topas/TOPASHeader.cc \
	src/topas/TOPASphspFile.cc \
	src/ROOT/ROOTphsp.cc \
	PHSPConvert.cc

GCC_SRCS_COMBINE := \
	src/PhaseSpaceFileReader.cc \
	src/PhaseSpaceFileWriter.cc \
	src/utilities/formats.cc \
	src/egs/egsphspFile.cc \
	src/peneasy/penEasyphspFile.cc \
	src/iaea/IAEAHeader.cc \
	src/iaea/IAEAphspFile.cc \
	src/topas/TOPASHeader.cc \
	src/topas/TOPASphspFile.cc \
	src/ROOT/ROOTphsp.cc \
	PHSPCombine.cc

GCC_SRCS_IMAGE := \
	src/PhaseSpaceFileReader.cc \
	src/PhaseSpaceFileWriter.cc \
	src/utilities/formats.cc \
	src/egs/egsphspFile.cc \
	src/peneasy/penEasyphspFile.cc \
	src/iaea/IAEAHeader.cc \
	src/iaea/IAEAphspFile.cc \
	src/topas/TOPASHeader.cc \
	src/topas/TOPASphspFile.cc \
	src/ROOT/ROOTphsp.cc \
	PHSPImage.cc

# detect Windows vs. Unix
ifeq ($(OS),Windows_NT)
    BINEXT := .exe
    MKDIR_P := mkdir
else
    BINEXT :=
    MKDIR_P := mkdir -p
endif

# Output dirs and binaries
GCC_BIN_DIR_REL := build/gcc/release
GCC_BIN_DIR_DBG := build/gcc/debug

CONVERT_BIN_REL := $(GCC_BIN_DIR_REL)/PHSPConvert$(BINEXT)
COMBINE_BIN_REL := $(GCC_BIN_DIR_REL)/PHSPCombine$(BINEXT)
IMAGE_BIN_REL := $(GCC_BIN_DIR_REL)/PHSPImage$(BINEXT)

CONVERT_BIN_DBG := $(GCC_BIN_DIR_DBG)/PHSPConvert$(BINEXT)
COMBINE_BIN_DBG := $(GCC_BIN_DIR_DBG)/PHSPCombine$(BINEXT)
IMAGE_BIN_DBG := $(GCC_BIN_DIR_DBG)/PHSPImage$(BINEXT)

# Make release the default goal
.DEFAULT_GOAL := release

.PHONY: release debug \
		gcc-release gcc-release-combine gcc-release-image \
		gcc-debug gcc-debug-combine gcc-release-image \
		clean

# Default (release)
release: gcc-release gcc-release-combine gcc-release-image

# Debug bundle
debug: gcc-debug gcc-debug-combine gcc-debug-image

# Release targets
gcc-release:
	@$(MKDIR_P) $(GCC_BIN_DIR_REL)
	@echo "Building GCC Release (PHSPConvert)..."
	$(CXX) $(CXXFLAGS_RELEASE) $(GCC_SRCS_CONVERT) -o $(CONVERT_BIN_REL) $(ROOT_LIBS)

gcc-release-combine:
	@$(MKDIR_P) $(GCC_BIN_DIR_REL)
	@echo "Building GCC Release (PHSPCombine)..."
	$(CXX) $(CXXFLAGS_RELEASE) $(GCC_SRCS_COMBINE) -o $(COMBINE_BIN_REL) $(ROOT_LIBS)

gcc-release-image:
	@$(MKDIR_P) $(GCC_BIN_DIR_REL)
	@echo "Building GCC Release (PHSPImage)..."
	$(CXX) $(CXXFLAGS_RELEASE) $(GCC_SRCS_IMAGE) -o $(IMAGE_BIN_REL) $(ROOT_LIBS)

# Debug targets
gcc-debug:
	@$(MKDIR_P) $(GCC_BIN_DIR_DBG)
	@echo "Building GCC Debug (PHSPConvert)..."
	$(CXX) $(CXXFLAGS_DEBUG) $(GCC_SRCS_CONVERT) -o $(CONVERT_BIN_DBG) $(ROOT_LIBS)

gcc-debug-combine:
	@$(MKDIR_P) $(GCC_BIN_DIR_DBG)
	@echo "Building GCC Debug (PHSPCombine)..."
	$(CXX) $(CXXFLAGS_DEBUG) $(GCC_SRCS_COMBINE) -o $(COMBINE_BIN_DBG) $(ROOT_LIBS)

gcc-debug-image:
	@$(MKDIR_P) $(GCC_BIN_DIR_DBG)
	@echo "Building GCC Debug (PHSPImage)..."
	$(CXX) $(CXXFLAGS_DEBUG) $(GCC_SRCS_IMAGE) -o $(IMAGE_BIN_DBG) $(ROOT_LIBS)

# Clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(GCC_BIN_DIR_REL) $(GCC_BIN_DIR_DBG)

# Installation directory (can be overridden)
PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin

.PHONY: install

install:
	@echo "Installing PHSPConvert and PHSPCombine and PHSPImage to $(BINDIR)"
	@mkdir -p $(BINDIR)
	@cp $(CONVERT_BIN_REL) $(BINDIR)
	@cp $(COMBINE_BIN_REL) $(BINDIR)
	@cp $(IMAGE_BIN_REL) $(BINDIR)


# load configuration
-include config.status

# Abort early if config.status is missing
ifneq ($(wildcard config.status),config.status)
  $(error config.status not found. Please run './configure' first.)
endif

# Enable parallel builds
JOBS   ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)
MAKEFLAGS += -j$(JOBS)

USE_ROOT ?= 0
ROOT_CFLAGS ?=
ROOT_LIBS   ?=
ROOT_SYS_CFLAGS := $(patsubst -I%,-isystem %,$(ROOT_CFLAGS))
ROOT_OTHER_FLAGS := $(filter-out -I%,$(ROOT_CFLAGS))

# only define USE_ROOT when actually enabled
ifeq ($(USE_ROOT),1)
    MACRO_DEFINE := -DUSE_ROOT=1
else
    MACRO_DEFINE :=
    ROOT_SYS_CFLAGS :=
    ROOT_OTHER_FLAGS :=
endif

# Common include flags
INCLUDES := -Iinclude

PZ_HEADERS := include/particlezoo

# Output dirs and binaries
GCC_BIN_DIR_REL := build/gcc/release
GCC_BIN_DIR_DBG := build/gcc/debug

# Source lists
GCC_SRCS_CONVERT := \
    src/PhaseSpaceFileReader.cc \
    src/PhaseSpaceFileWriter.cc \
    src/utilities/formats.cc \
    src/utilities/argParse.cc \
    src/egs/egsphspFile.cc \
    src/peneasy/penEasyphspFile.cc \
    src/IAEA/IAEAHeader.cc \
    src/IAEA/IAEAphspFile.cc \
    src/topas/TOPASHeader.cc \
    src/topas/TOPASphspFile.cc \
    src/ROOT/ROOTphsp.cc \
    PHSPConvert.cc

GCC_SRCS_COMBINE := \
    src/PhaseSpaceFileReader.cc \
    src/PhaseSpaceFileWriter.cc \
    src/utilities/formats.cc \
    src/utilities/argParse.cc \
    src/egs/egsphspFile.cc \
    src/peneasy/penEasyphspFile.cc \
    src/IAEA/IAEAHeader.cc \
    src/IAEA/IAEAphspFile.cc \
    src/topas/TOPASHeader.cc \
    src/topas/TOPASphspFile.cc \
    src/ROOT/ROOTphsp.cc \
    PHSPCombine.cc

GCC_SRCS_IMAGE := \
    src/PhaseSpaceFileReader.cc \
    src/PhaseSpaceFileWriter.cc \
    src/utilities/formats.cc \
    src/utilities/argParse.cc \
    src/egs/egsphspFile.cc \
    src/peneasy/penEasyphspFile.cc \
    src/IAEA/IAEAHeader.cc \
    src/IAEA/IAEAphspFile.cc \
    src/topas/TOPASHeader.cc \
    src/topas/TOPASphspFile.cc \
    src/ROOT/ROOTphsp.cc \
    PHSPImage.cc

# --- static library settings ---
LIB_NAME := libparticlezoo.a
LIB_SRCS := \
        src/PhaseSpaceFileReader.cc \
        src/PhaseSpaceFileWriter.cc \
        src/utilities/formats.cc \
        src/utilities/argParse.cc \
        src/egs/egsphspFile.cc \
        src/peneasy/penEasyphspFile.cc \
        src/IAEA/IAEAHeader.cc \
        src/IAEA/IAEAphspFile.cc \
        src/topas/TOPASHeader.cc \
        src/topas/TOPASphspFile.cc \
        src/ROOT/ROOTphsp.cc

LIB_REL := $(GCC_BIN_DIR_REL)/$(LIB_NAME)
LIB_DBG := $(GCC_BIN_DIR_DBG)/$(LIB_NAME)

# Allow pattern rules to find .cc in src/ and project root
vpath %.cc src .

# Optionally include external submodule overrides
-include ext/ext.mk

LIB_OBJS_REL := $(patsubst %.cc,$(GCC_BIN_DIR_REL)/%.o,$(LIB_SRCS))
LIB_OBJS_DBG := $(patsubst %.cc,$(GCC_BIN_DIR_DBG)/%.o,$(LIB_SRCS))

# Release flags
CXXFLAGS_RELEASE := $(CXXFLAGS) -O3 -march=native $(MACRO_DEFINE) $(INCLUDES) $(ROOT_SYS_CFLAGS) $(ROOT_OTHER_FLAGS)

# Debug flags
CXXFLAGS_DEBUG := $(CXXFLAGS) -O0 -g $(MACRO_DEFINE) $(INCLUDES) $(ROOT_SYS_CFLAGS) $(ROOT_OTHER_FLAGS)

# detect Windows vs. Unix
ifeq ($(OS),Windows_NT)
    BINEXT := .exe
    MKDIR_P := mkdir
else
    BINEXT :=
    MKDIR_P := mkdir -p
endif

CONVERT_BIN_REL := $(GCC_BIN_DIR_REL)/PHSPConvert$(BINEXT)
COMBINE_BIN_REL := $(GCC_BIN_DIR_REL)/PHSPCombine$(BINEXT)
IMAGE_BIN_REL   := $(GCC_BIN_DIR_REL)/PHSPImage$(BINEXT)

CONVERT_BIN_DBG := $(GCC_BIN_DIR_DBG)/PHSPConvert$(BINEXT)
COMBINE_BIN_DBG := $(GCC_BIN_DIR_DBG)/PHSPCombine$(BINEXT)
IMAGE_BIN_DBG   := $(GCC_BIN_DIR_DBG)/PHSPImage$(BINEXT)

# Make release the default goal
.DEFAULT_GOAL := release

.PHONY: release debug \
        gcc-release-convert gcc-release-combine gcc-release-image gcc-release-lib \
        gcc-debug-convert   gcc-debug-combine   gcc-debug-image gcc-debug-lib \
        clean install install-debug

# Default (release)
release: gcc-release-convert gcc-release-combine gcc-release-image gcc-release-lib

# Debug bundle
debug: gcc-debug-convert gcc-debug-combine gcc-debug-image gcc-debug-lib

# Object lists for executables
CONVERT_OBJS_REL := $(patsubst %.cc,$(GCC_BIN_DIR_REL)/%.o,$(GCC_SRCS_CONVERT))
COMBINE_OBJS_REL := $(patsubst %.cc,$(GCC_BIN_DIR_REL)/%.o,$(GCC_SRCS_COMBINE))
IMAGE_OBJS_REL   := $(patsubst %.cc,$(GCC_BIN_DIR_REL)/%.o,$(GCC_SRCS_IMAGE))

# Release executable targets
gcc-release-convert: $(CONVERT_BIN_REL)
gcc-release-combine: $(COMBINE_BIN_REL)
gcc-release-image:   $(IMAGE_BIN_REL)

$(CONVERT_BIN_REL): $(CONVERT_OBJS_REL)
	@$(MKDIR_P) $(dir $@)
	@echo "Linking Release (PHSPConvert)…"
	$(CXX) $(CXXFLAGS_RELEASE) $^ -o $@ $(ROOT_LIBS)
	@echo " "

$(COMBINE_BIN_REL): $(COMBINE_OBJS_REL)
	@$(MKDIR_P) $(dir $@)
	@echo "Linking Release (PHSPCombine)…"
	$(CXX) $(CXXFLAGS_RELEASE) $^ -o $@ $(ROOT_LIBS)

$(IMAGE_BIN_REL): $(IMAGE_OBJS_REL)
	@$(MKDIR_P) $(dir $@)
	@echo "Linking Release (PHSPImage)…"
	$(CXX) $(CXXFLAGS_RELEASE) $^ -o $@ $(ROOT_LIBS)

# Release static library
gcc-release-lib: $(LIB_REL)
$(LIB_REL): $(LIB_OBJS_REL)
	@$(MKDIR_P) $(dir $@)
	@echo "Building Release static library ($@)…"
	ar rcs $@ $^

# Debug executable targets (could be parallelized similarly)
gcc-debug-convert:
	@$(MKDIR_P) $(GCC_BIN_DIR_DBG)
	@echo "Building Debug (PHSPConvert)..."
	$(CXX) $(CXXFLAGS_DEBUG) $(GCC_SRCS_CONVERT) -o $(CONVERT_BIN_DBG) $(ROOT_LIBS)

gcc-debug-combine:
	@$(MKDIR_P) $(GCC_BIN_DIR_DBG)
	@echo "Building Debug (PHSPCombine)..."
	$(CXX) $(CXXFLAGS_DEBUG) $(GCC_SRCS_COMBINE) -o $(COMBINE_BIN_DBG) $(ROOT_LIBS)

gcc-debug-image:
	@$(MKDIR_P) $(GCC_BIN_DIR_DBG)
	@echo "Building Debug (PHSPImage)..."
	$(CXX) $(CXXFLAGS_DEBUG) $(GCC_SRCS_IMAGE) -o $(IMAGE_BIN_DBG) $(ROOT_LIBS)

gcc-debug-lib: $(LIB_DBG)
$(LIB_DBG): $(LIB_OBJS_DBG)
	@$(MKDIR_P) $(dir $@)
	@echo "Building Debug static library ($@)…"
	ar rcs $@ $^

# --- compile object files into the right dirs ---
$(GCC_BIN_DIR_REL)/%.o: %.cc
	@$(MKDIR_P) $(dir $@)
	@echo "Compiling Release object $<…"
	$(CXX) $(CXXFLAGS_RELEASE) -c $< -o $@

$(GCC_BIN_DIR_DBG)/%.o: %.cc
	@$(MKDIR_P) $(dir $@)
	@echo "Compiling Debug object $<…"
	$(CXX) $(CXXFLAGS_DEBUG) -c $< -o $@

# Clean
clean:
	@printf "Cleaning build artifacts..."
	@rm -rf $(GCC_BIN_DIR_REL) $(GCC_BIN_DIR_DBG)
	@echo " done."

# Installation directory (can be overridden)
PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin
LIBDIR := $(PREFIX)/lib

install:
	@printf "Installing into $(BINDIR), $(LIBDIR) and headers into $(PREFIX)/include..."
	@$(MKDIR_P) $(BINDIR) $(LIBDIR) $(PREFIX)/include
	@cp $(CONVERT_BIN_REL) $(COMBINE_BIN_REL) $(IMAGE_BIN_REL) $(BINDIR)
	@cp $(LIB_REL) $(LIBDIR)
	@cp -r $(PZ_HEADERS) $(PREFIX)/include
	@echo " done."

install-debug:
	@printf "Installing debug binaries and library to $(BINDIR), $(LIBDIR) and headers into $(PREFIX)/include..."
	@$(MKDIR_P) $(BINDIR) $(LIBDIR) $(PREFIX)/include
	@cp $(CONVERT_BIN_DBG) $(COMBINE_BIN_DBG) $(IMAGE_BIN_DBG) $(BINDIR)
	@cp $(LIB_DBG) $(LIBDIR)
	@cp -r $(PZ_HEADERS) $(PREFIX)/include
	@echo " done."

uninstall:
	@printf "Removing particlezoo installation from $(PREFIX)..."
	@rm -f $(BINDIR)/PHSPConvert$(BINEXT) $(BINDIR)/PHSPCombine$(BINEXT) $(BINDIR)/PHSPImage$(BINEXT)
	@rm -f $(LIBDIR)/$(LIB_NAME)
	@rm -rf $(PREFIX)/include/particlezoo
	@echo " done."
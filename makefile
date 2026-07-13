# --- Windows Redirection ---
ifeq ($(OS),Windows_NT)
  # Default goal for Windows
  .DEFAULT_GOAL := release

  # Force cmd.exe as the recipe shell so build.bat forwarding works even
  # when a POSIX sh.exe is present in PATH (e.g. from Git for Windows)
  SHELL := cmd.exe
  .SHELLFLAGS := /C

  # Map common make variables to build.bat flags
  WIN_OPTS :=
  ifdef PREFIX
    WIN_OPTS += --prefix "$(PREFIX)"
  endif
  ifeq ($(USE_ROOT),0)
    WIN_OPTS += --no-root
  endif
  ifdef JOBS
    WIN_OPTS += --jobs $(JOBS)
  endif

  release:
	@call build.bat release $(WIN_OPTS)

  debug:
	@call build.bat debug $(WIN_OPTS)

  install:
	@call build.bat install $(WIN_OPTS)

  clean:
	@if exist build\msvc (echo Cleaning build artifacts... && rmdir /s /q build\msvc && echo Done.)

  .PHONY: release debug install clean

else
# --- POSIX (Linux/macOS) Logic ---

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
    USE_ROOT := 0
endif

# Common include flags
INCLUDES := -Iinclude

PZ_HEADERS := include/particlezoo

# Output dirs and binaries
GCC_BIN_DIR_REL := build/gcc/release
GCC_BIN_DIR_DBG := build/gcc/debug

# Source lists
# COMMON_SRCS holds every translation unit shared by the executables and the
# library; add new formats here once instead of once per target list.
COMMON_SRCS := \
    src/PhaseSpaceFileReader.cc \
    src/PhaseSpaceFileWriter.cc \
    src/utilities/formats.cc \
    src/utilities/argParse.cc \
    src/egs/egsphspFile.cc \
    src/peneasy/penEasyphspFile.cc \
    src/IAEA/IAEAHeader.cc \
    src/IAEA/IAEAphspFile.cc \
    src/MCNP/MCNPphspFile.cc \
    src/mcpl/MCPLphspFile.cc \
    src/topas/TOPASHeader.cc \
    src/topas/TOPASphspFile.cc \
    src/ROOT/ROOTphsp.cc

GCC_SRCS_CONVERT := $(COMMON_SRCS) src/operations/Convert.cc PHSPConvert.cc
GCC_SRCS_COMBINE := $(COMMON_SRCS) src/operations/Combine.cc PHSPCombine.cc
GCC_SRCS_IMAGE   := $(COMMON_SRCS) src/operations/GenerateImage.cc PHSPImage.cc
GCC_SRCS_SPLIT   := $(COMMON_SRCS) src/operations/Split.cc PHSPSplit.cc

# --- static library settings ---
LIB_NAME := libparticlezoo.a
LIB_SRCS := $(COMMON_SRCS) \
        src/parallel/ParticleBalancedParallelReader.cc \
        src/parallel/HistoryBalancedParallelReader.cc \
        src/operations/Combine.cc \
        src/operations/Convert.cc \
        src/operations/GenerateImage.cc \
        src/operations/Split.cc

LIB_REL := $(GCC_BIN_DIR_REL)/$(LIB_NAME)
LIB_DBG := $(GCC_BIN_DIR_DBG)/$(LIB_NAME)

# Allow pattern rules to find .cc in src/ and project root
vpath %.cc src .

# Optionally include external submodule overrides
-include ext/ext.mk

LIB_OBJS_REL := $(patsubst %.cc,$(GCC_BIN_DIR_REL)/%.o,$(LIB_SRCS))
LIB_OBJS_DBG := $(patsubst %.cc,$(GCC_BIN_DIR_DBG)/%.o,$(LIB_SRCS))

# Release flags
# ARCHFLAGS may be overridden for portable/distribution builds (e.g. ARCHFLAGS=-mtune=generic)
ARCHFLAGS ?= -march=native
CXXFLAGS_RELEASE := $(CXXFLAGS) -O3 $(ARCHFLAGS) -Wno-deprecated-declarations $(MACRO_DEFINE) $(INCLUDES) $(ROOT_SYS_CFLAGS) $(ROOT_OTHER_FLAGS)

# Debug flags
CXXFLAGS_DEBUG := $(CXXFLAGS) -O0 -g -Wno-deprecated-declarations $(MACRO_DEFINE) $(INCLUDES) $(ROOT_SYS_CFLAGS) $(ROOT_OTHER_FLAGS)

# PIC flags for shared library
CXXFLAGS_RELEASE_PIC := $(CXXFLAGS_RELEASE) -fPIC
CXXFLAGS_DEBUG_PIC   := $(CXXFLAGS_DEBUG) -fPIC

# Unix platform detection (OS is not Windows_NT)
BINEXT :=
MKDIR_P := mkdir -p
UNAME_S := $(shell uname -s)

# Library version, read from the canonical source in version.h.
# SOVERSION is the ABI major number embedded in the SONAME; it changes only
# on ABI-breaking releases, not with every version bump.
VERSION := $(shell awk '/MAJOR_VERSION[ \t]*=/{ma=$$NF+0} /MINOR_VERSION[ \t]*=/{mi=$$NF+0} /PATCH_VERSION[ \t]*=/{pa=$$NF+0} END{printf "%d.%d.%d", ma, mi, pa}' include/particlezoo/utilities/version.h)
SOVERSION := $(word 1,$(subst ., ,$(VERSION)))

ifeq ($(UNAME_S),Darwin)
    SHLIB_EXT := .dylib
    SHLIB_FLAG := -dynamiclib
    SHLIB_SONAME := libparticlezoo.$(SOVERSION)$(SHLIB_EXT)
    SHLIB_REALNAME := libparticlezoo.$(VERSION)$(SHLIB_EXT)
    # LIBDIR is expanded at link time (recursive assignment), so the
    # install_name reflects the PREFIX the library will be installed under.
    SHLIB_LDFLAGS = -install_name $(LIBDIR)/$(SHLIB_SONAME) -current_version $(VERSION) -compatibility_version $(SOVERSION).0
else
    SHLIB_EXT := .so
    SHLIB_FLAG := -shared
    SHLIB_SONAME := libparticlezoo$(SHLIB_EXT).$(SOVERSION)
    SHLIB_REALNAME := libparticlezoo$(SHLIB_EXT).$(VERSION)
    SHLIB_LDFLAGS := -Wl,-soname,$(SHLIB_SONAME)
endif

CONVERT_BIN_REL := $(GCC_BIN_DIR_REL)/PHSPConvert$(BINEXT)
COMBINE_BIN_REL := $(GCC_BIN_DIR_REL)/PHSPCombine$(BINEXT)
IMAGE_BIN_REL   := $(GCC_BIN_DIR_REL)/PHSPImage$(BINEXT)
SPLIT_BIN_REL   := $(GCC_BIN_DIR_REL)/PHSPSplit$(BINEXT)

CONVERT_BIN_DBG := $(GCC_BIN_DIR_DBG)/PHSPConvert$(BINEXT)
COMBINE_BIN_DBG := $(GCC_BIN_DIR_DBG)/PHSPCombine$(BINEXT)
IMAGE_BIN_DBG   := $(GCC_BIN_DIR_DBG)/PHSPImage$(BINEXT)
SPLIT_BIN_DBG   := $(GCC_BIN_DIR_DBG)/PHSPSplit$(BINEXT)

SHLIB_NAME := libparticlezoo$(SHLIB_EXT)
SHLIB_REL := $(GCC_BIN_DIR_REL)/$(SHLIB_NAME)
SHLIB_DBG := $(GCC_BIN_DIR_DBG)/$(SHLIB_NAME)

# Make release the default goal
.DEFAULT_GOAL := release

.PHONY: release debug \
        gcc-release-convert gcc-release-combine gcc-release-image gcc-release-split gcc-release-lib gcc-release-shlib \
        gcc-debug-convert   gcc-debug-combine   gcc-debug-image gcc-debug-split gcc-debug-lib gcc-debug-shlib \
        clean install install-debug install-python install-python-dev uninstall-python

# Default (release)
release: gcc-release-convert gcc-release-combine gcc-release-image gcc-release-split gcc-release-lib gcc-release-shlib

# Debug bundle
debug: gcc-debug-convert gcc-debug-combine gcc-debug-image gcc-debug-split gcc-debug-lib gcc-debug-shlib

# Release object lists for executables
CONVERT_OBJS_REL := $(patsubst %.cc,$(GCC_BIN_DIR_REL)/%.o,$(GCC_SRCS_CONVERT))
COMBINE_OBJS_REL := $(patsubst %.cc,$(GCC_BIN_DIR_REL)/%.o,$(GCC_SRCS_COMBINE))
IMAGE_OBJS_REL   := $(patsubst %.cc,$(GCC_BIN_DIR_REL)/%.o,$(GCC_SRCS_IMAGE))
SPLIT_OBJS_REL   := $(patsubst %.cc,$(GCC_BIN_DIR_REL)/%.o,$(GCC_SRCS_SPLIT))

# Debug object lists for executables
CONVERT_OBJS_DBG := $(patsubst %.cc,$(GCC_BIN_DIR_DBG)/%.o,$(GCC_SRCS_CONVERT))
COMBINE_OBJS_DBG := $(patsubst %.cc,$(GCC_BIN_DIR_DBG)/%.o,$(GCC_SRCS_COMBINE))
IMAGE_OBJS_DBG   := $(patsubst %.cc,$(GCC_BIN_DIR_DBG)/%.o,$(GCC_SRCS_IMAGE))
SPLIT_OBJS_DBG   := $(patsubst %.cc,$(GCC_BIN_DIR_DBG)/%.o,$(GCC_SRCS_SPLIT))

# Shared library object lists (compiled with -fPIC)
SHLIB_OBJS_REL := $(patsubst %.cc,$(GCC_BIN_DIR_REL)/pic/%.o,$(LIB_SRCS))
SHLIB_OBJS_DBG := $(patsubst %.cc,$(GCC_BIN_DIR_DBG)/pic/%.o,$(LIB_SRCS))

# Release executable targets
gcc-release-convert: $(CONVERT_BIN_REL)
gcc-release-combine: $(COMBINE_BIN_REL)
gcc-release-image:   $(IMAGE_BIN_REL)
gcc-release-split:   $(SPLIT_BIN_REL)

$(CONVERT_BIN_REL): $(CONVERT_OBJS_REL)
	@$(MKDIR_P) $(dir $@)
	@echo "Linking Release (PHSPConvert)..."
	$(CXX) $(CXXFLAGS_RELEASE) $^ -o $@ $(ROOT_LIBS)
	@echo " "

$(COMBINE_BIN_REL): $(COMBINE_OBJS_REL)
	@$(MKDIR_P) $(dir $@)
	@echo "Linking Release (PHSPCombine)..."
	$(CXX) $(CXXFLAGS_RELEASE) $^ -o $@ $(ROOT_LIBS)

$(IMAGE_BIN_REL): $(IMAGE_OBJS_REL)
	@$(MKDIR_P) $(dir $@)
	@echo "Linking Release (PHSPImage)..."
	$(CXX) $(CXXFLAGS_RELEASE) $^ -o $@ $(ROOT_LIBS)

$(SPLIT_BIN_REL): $(SPLIT_OBJS_REL)
	@$(MKDIR_P) $(dir $@)
	@echo "Linking Release (PHSPSplit)..."
	$(CXX) $(CXXFLAGS_RELEASE) $^ -o $@ $(ROOT_LIBS)

# Release static library
gcc-release-lib: $(LIB_REL)
$(LIB_REL): $(LIB_OBJS_REL)
	@$(MKDIR_P) $(dir $@)
	@echo "Building Release static library ($@)..."
	ar rcs $@ $^

# Release shared library
gcc-release-shlib: $(SHLIB_REL)
$(SHLIB_REL): $(SHLIB_OBJS_REL)
	@$(MKDIR_P) $(dir $@)
	@echo "Building Release shared library ($@)..."
	$(CXX) $(SHLIB_FLAG) $(SHLIB_LDFLAGS) -o $@ $^ $(ROOT_LIBS)

# Debug executable targets (could be parallelized similarly)
gcc-debug-convert: $(CONVERT_OBJS_DBG)
	@$(MKDIR_P) $(GCC_BIN_DIR_DBG)
	@echo "Building Debug (PHSPConvert)..."
	$(CXX) $(CXXFLAGS_DEBUG) $(CONVERT_OBJS_DBG) -o $(CONVERT_BIN_DBG) $(ROOT_LIBS)

gcc-debug-combine: $(COMBINE_OBJS_DBG)
	@$(MKDIR_P) $(GCC_BIN_DIR_DBG)
	@echo "Building Debug (PHSPCombine)..."
	$(CXX) $(CXXFLAGS_DEBUG) $(COMBINE_OBJS_DBG) -o $(COMBINE_BIN_DBG) $(ROOT_LIBS)

gcc-debug-image: $(IMAGE_OBJS_DBG)
	@$(MKDIR_P) $(GCC_BIN_DIR_DBG)
	@echo "Building Debug (PHSPImage)..."
	$(CXX) $(CXXFLAGS_DEBUG) $(IMAGE_OBJS_DBG) -o $(IMAGE_BIN_DBG) $(ROOT_LIBS)

gcc-debug-split: $(SPLIT_OBJS_DBG)
	@$(MKDIR_P) $(GCC_BIN_DIR_DBG)
	@echo "Building Debug (PHSPSplit)..."
	$(CXX) $(CXXFLAGS_DEBUG) $(SPLIT_OBJS_DBG) -o $(SPLIT_BIN_DBG) $(ROOT_LIBS)

gcc-debug-lib: $(LIB_DBG)
$(LIB_DBG): $(LIB_OBJS_DBG)
	@$(MKDIR_P) $(dir $@)
	@echo "Building Debug static library ($@)..."
	ar rcs $@ $^

# Debug shared library
gcc-debug-shlib: $(SHLIB_DBG)
$(SHLIB_DBG): $(SHLIB_OBJS_DBG)
	@$(MKDIR_P) $(dir $@)
	@echo "Building Debug shared library ($@)..."
	$(CXX) $(SHLIB_FLAG) $(SHLIB_LDFLAGS) -o $@ $^ $(ROOT_LIBS)

# --- compile object files into the right dirs ---
$(GCC_BIN_DIR_REL)/%.o: %.cc
	@$(MKDIR_P) $(dir $@)
	@echo "Compiling Release object $<..."
	$(CXX) $(CXXFLAGS_RELEASE) -c $< -o $@

$(GCC_BIN_DIR_DBG)/%.o: %.cc
	@$(MKDIR_P) $(dir $@)
	@echo "Compiling Debug object $<..."
	$(CXX) $(CXXFLAGS_DEBUG) -c $< -o $@

$(GCC_BIN_DIR_REL)/pic/%.o: %.cc
	@$(MKDIR_P) $(dir $@)
	@echo "Compiling Release PIC object $<..."
	$(CXX) $(CXXFLAGS_RELEASE_PIC) -c $< -o $@

$(GCC_BIN_DIR_DBG)/pic/%.o: %.cc
	@$(MKDIR_P) $(dir $@)
	@echo "Compiling Debug PIC object $<..."
	$(CXX) $(CXXFLAGS_DEBUG_PIC) -c $< -o $@

# Clean
clean:
	@printf "Cleaning build artifacts..."
	@rm -rf $(GCC_BIN_DIR_REL) $(GCC_BIN_DIR_DBG)
	@echo " done."

# Installation directories (can be overridden; DESTDIR supports staged
# installs for package builds, e.g. make install DESTDIR=/tmp/stage)
PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin
LIBDIR := $(PREFIX)/lib
PCDIR  := $(LIBDIR)/pkgconfig
MANDIR := $(PREFIX)/share/man/man1

MANPAGES := docs/man/PHSPConvert.1 docs/man/PHSPCombine.1 docs/man/PHSPImage.1 docs/man/PHSPSplit.1

# Install man pages with the version substituted into their headers
define INSTALL_MANPAGES
	$(MKDIR_P) $(DESTDIR)$(MANDIR)
	for page in $(MANPAGES); do \
	  sed 's|@VERSION@|$(VERSION)|g' $$page > $(DESTDIR)$(MANDIR)/$$(basename $$page); \
	done
endef

# Generate the pkg-config file from its template
define INSTALL_PKGCONFIG
	$(MKDIR_P) $(DESTDIR)$(PCDIR)
	sed -e 's|@prefix@|$(PREFIX)|g' \
	    -e 's|@libdir@|$(LIBDIR)|g' \
	    -e 's|@version@|$(VERSION)|g' \
	    particlezoo.pc.in > $(DESTDIR)$(PCDIR)/particlezoo.pc
endef

# Install the shared library under its versioned name with the SONAME and
# development symlinks alongside it
define INSTALL_SHLIB
	cp $(1) $(DESTDIR)$(LIBDIR)/$(SHLIB_REALNAME)
	ln -sf $(SHLIB_REALNAME) $(DESTDIR)$(LIBDIR)/$(SHLIB_SONAME)
	ln -sf $(SHLIB_SONAME) $(DESTDIR)$(LIBDIR)/$(SHLIB_NAME)
endef

install:
	@printf "Installing into $(DESTDIR)$(BINDIR), $(DESTDIR)$(LIBDIR) and headers into $(DESTDIR)$(PREFIX)/include..."
	@$(MKDIR_P) $(DESTDIR)$(BINDIR) $(DESTDIR)$(LIBDIR) $(DESTDIR)$(PREFIX)/include
	@cp $(CONVERT_BIN_REL) $(COMBINE_BIN_REL) $(IMAGE_BIN_REL) $(SPLIT_BIN_REL) $(DESTDIR)$(BINDIR)
	@cp $(LIB_REL) $(DESTDIR)$(LIBDIR)
	@$(call INSTALL_SHLIB,$(SHLIB_REL))
	@$(call INSTALL_PKGCONFIG)
	@$(call INSTALL_MANPAGES)
	@cp -r $(PZ_HEADERS) $(DESTDIR)$(PREFIX)/include
	@echo " done."

install-debug:
	@printf "Installing debug binaries and library to $(DESTDIR)$(BINDIR), $(DESTDIR)$(LIBDIR) and headers into $(DESTDIR)$(PREFIX)/include..."
	@$(MKDIR_P) $(DESTDIR)$(BINDIR) $(DESTDIR)$(LIBDIR) $(DESTDIR)$(PREFIX)/include
	@cp $(CONVERT_BIN_DBG) $(COMBINE_BIN_DBG) $(IMAGE_BIN_DBG) $(SPLIT_BIN_DBG) $(DESTDIR)$(BINDIR)
	@cp $(LIB_DBG) $(DESTDIR)$(LIBDIR)
	@$(call INSTALL_SHLIB,$(SHLIB_DBG))
	@$(call INSTALL_PKGCONFIG)
	@$(call INSTALL_MANPAGES)
	@cp -r $(PZ_HEADERS) $(DESTDIR)$(PREFIX)/include
	@echo " done."

uninstall:
	@printf "Removing particlezoo installation from $(DESTDIR)$(PREFIX)..."
	@rm -f $(DESTDIR)$(BINDIR)/PHSPConvert$(BINEXT) $(DESTDIR)$(BINDIR)/PHSPCombine$(BINEXT) $(DESTDIR)$(BINDIR)/PHSPImage$(BINEXT) $(DESTDIR)$(BINDIR)/PHSPSplit$(BINEXT)
	@rm -f $(DESTDIR)$(LIBDIR)/$(LIB_NAME) $(DESTDIR)$(LIBDIR)/$(SHLIB_NAME) $(DESTDIR)$(LIBDIR)/$(SHLIB_SONAME) $(DESTDIR)$(LIBDIR)/$(SHLIB_REALNAME)
	@rm -f $(DESTDIR)$(PCDIR)/particlezoo.pc
	@rm -f $(DESTDIR)$(MANDIR)/PHSPConvert.1 $(DESTDIR)$(MANDIR)/PHSPCombine.1 $(DESTDIR)$(MANDIR)/PHSPImage.1 $(DESTDIR)$(MANDIR)/PHSPSplit.1
	@rm -rf $(DESTDIR)$(PREFIX)/include/particlezoo
	@echo " done."


# Python installation targets

install-python:
	@if [ -n "$$VIRTUAL_ENV" ]; then \
		printf "Installing Python bindings to virtual environment..."; \
		cd python && pip install .; \
		echo " done."; \
	else \
		printf "Installing Python bindings to virtual environment..."; \
		echo "WARNING: No virtual environment detected. Installing instead to user site-packages (~/.local)."; \
		echo "This uses --break-system-packages to bypass externally-managed-environment protection."; \
		printf "Continue? [y/N] " && read ans && [ $${ans:-N} = y ] || { echo "Canceled."; exit 1; }; \
		printf "Installing Python bindings to user site-packages..."; \
		cd python && pip install --user --break-system-packages .; \
		echo " done."; \
	fi

install-python-dev:
	@if [ -n "$$VIRTUAL_ENV" ]; then \
		printf "Installing Python bindings in development mode..."; \
		cd python && pip install -e .; \
		echo " done."; \
	else \
		echo "Installing Python bindings in development mode..."; \
		echo "WARNING: No virtual environment detected. Installing to user site-packages (~/.local)."; \
		echo "This uses --break-system-packages to bypass externally-managed-environment protection."; \
		printf "Continue? [y/N] " && read ans && [ $${ans:-N} = y ] || { echo "Canceled."; exit 1; }; \
		printf "Installing Python bindings in development mode..."; \
		cd python && pip install --user --break-system-packages -e .; \
		echo " done."; \
	fi

uninstall-python:
	@printf "Attempting to uninstall particlezoo..."
	@if pip uninstall -y particlezoo 2>/dev/null; then \
		echo " done."; \
	else \
		echo " failed."; \
		echo "WARNING: Standard uninstall failed. May need --break-system-packages flag."; \
		printf "Try with --break-system-packages? [y/N] " && read ans; \
		if [ "$${ans:-N}" = "y" ]; then \
			pip uninstall --break-system-packages -y particlezoo && echo "Done." || echo "Failed."; \
		else \
			echo "Canceled."; \
		fi; \
	fi
endif
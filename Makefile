T := $(realpath $(dir $(filter %Makefile,$(MAKEFILE_LIST))))

# List of supported boards
SUPPORTED_BOARDS := nucleo_l552ze_q b_u585i_iot02a

# Default board
BOARD ?= nucleo_l552ze_q

# Validate board selection
ifneq ($(BOARD),$(filter $(BOARD),$(SUPPORTED_BOARDS)))
  $(error Unsupported board: $(BOARD). Supported boards are: $(SUPPORTED_BOARDS))
endif

# Build Configuration
BUILD ?= $(T)/build/$(BOARD)
TYPE  ?= Debug
SYSROOT := $(BUILD)/$(TYPE)/sysroot

# Tools sysroot (shared across build types)
TOOLS_SYSROOT := $(T)/build/tools/sysroot

CMAKE_PREFIX_PATH := \
  $(TOOLS_SYSROOT)/usr/local; \
  $(SYSROOT)/usr/local

.PHONY: all toolchain hal configure build clean distclean help
.PHONY: list-boards build-all

all: toolchain hal configure build tools

toolchain:
	$(MAKE) -C toolchain

hal:
	git submodule sync --recursive
	git submodule update --init --recursive

configure: toolchain hal
	@echo "==> Configuring for board: $(BOARD) ($(TYPE) build)"
	cmake -S . -B $(BUILD)/$(TYPE) \
		-DCMAKE_BUILD_TYPE=$(TYPE) \
		-DBOARD=$(BOARD) \
		-DBUILD_SYSROOT=$(SYSROOT) \
		-DCMAKE_PREFIX_PATH="$(CMAKE_PREFIX_PATH)" \
		-DCMAKE_TOOLCHAIN_FILE=toolchain/arm-none-eabi.cmake


tools:
	$(MAKE) -C ext BUILD_SYSROOT=$(TOOLS_SYSROOT)

build: configure tools
	@echo "==> Building for board: $(BOARD)"
	cmake --build $(BUILD)/$(TYPE) --verbose

clean:
	@echo "==> Cleaning build for board: $(BOARD)"
	rm -rf $(BUILD)

distclean:
	@echo "==> Cleaning all boards and tools"
	$(MAKE) -C ext clean
	rm -rf $(T)/build

# Build for all supported boards
build-all:
	@echo "==> Building for all supported boards"
	@for board in $(SUPPORTED_BOARDS); do \
		echo ""; \
		echo "Building for $$board..."; \
		$(MAKE) BOARD=$$board clean build || exit 1; \
	done
	@echo "==> Build complete for all boards"

# List available boards
list-boards:
	@echo "Supported boards:"
	@for board in $(SUPPORTED_BOARDS); do \
		echo "  - $$board"; \
	done

# Help Target
help:
	@echo "STM32 Application Build System"
	@echo ""
	@echo "Usage: make [BOARD=<board>] [TYPE=<type>]"
	@echo ""
	@echo "Boards:"
	@echo "  BOARD=nucleo_l552ze_q    Nucleo L552ZE-Q (default)"
	@echo "  BOARD=b_u585i_iot02a     B-U585I-IOT02A Discovery Kit"
	@echo ""
	@echo "Build Types:"
	@echo "  TYPE=Debug               Debug build with symbols (default)"
	@echo "  TYPE=Release             Optimized release build"
	@echo ""
	@echo "Targets:"
	@echo "  make all                 Configure, build toolchain and app (default)"
	@echo "  make configure           Run CMake configuration step"
	@echo "  make build               Build the application"
	@echo "  make clean               Clean build artifacts for current board"
	@echo "  make distclean           Clean all boards and tools"
	@echo "  make build-all           Build for all supported boards"
	@echo "  make list-boards         List all supported boards"
	@echo "  make help                Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make                                    	# Build for default board (nucleo_l552ze_q)"
	@echo "  make BOARD=nucleo_l552ze_q TYPE=Debug		# Build for Nucleo L552ZE-Q"
	@echo "  make BOARD=nucleo_l552ze_q TYPE=Release  	# Release build for nucleo_l552ze_q"
	@echo "  make BOARD=b_u585i_iot02a TYPE=Debug    	# Build for B-U585I-IOT02A"
	@echo "  make build-all                          	# Build for all boards"

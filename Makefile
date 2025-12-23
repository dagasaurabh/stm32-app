T := $(realpath $(dir $(filter %Makefile,$(MAKEFILE_LIST))))

BOARD ?= nucleo_l552ze_q
BUILD ?= $(T)/build/$(BOARD)
TYPE  ?= Debug
SYSROOT := $(BUILD)/$(TYPE)/sysroot

# Tools sysroot (shared across build types)
TOOLS_SYSROOT := $(T)/build/$(BOARD)/tools/sysroot

CMAKE_PREFIX_PATH := \
  $(TOOLS_SYSROOT)/usr/local; \
  $(SYSROOT)/usr/local

.PHONY: all toolchain hal configure build flash clean

all: toolchain hal configure build tools

toolchain:
	$(MAKE) -C toolchain

hal:
	git submodule sync --recursive
	git submodule update --init --recursive

configure: toolchain hal
	cmake -S . -B $(BUILD)/$(TYPE) \
		-DCMAKE_BUILD_TYPE=$(TYPE) \
		-DBOARD=$(BOARD) \
		-DBUILD_SYSROOT=$(SYSROOT) \
		-DCMAKE_PREFIX_PATH="$(CMAKE_PREFIX_PATH)" \
		-DCMAKE_TOOLCHAIN_FILE=toolchain/arm-none-eabi.cmake


tools:
	$(MAKE) -C ext BUILD_SYSROOT=$(TOOLS_SYSROOT)

build: configure tools
	cmake --build $(BUILD)/$(TYPE) --verbose

clean:
	$(MAKE) -C ext clean
	rm -rf build


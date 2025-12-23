OPENOCD_VERSION ?= v0.12.0
OPENOCD_REPO    ?= https://git.code.sf.net/p/openocd/code

# BUILD_SYSROOT must be passed from top-level Makefile
ifndef BUILD_SYSROOT
$(error BUILD_SYSROOT is not set)
endif

$(shell mkdir -p $(BUILD_SYSROOT))
ABS_SYSROOT := $(realpath $(BUILD_SYSROOT))
PREFIX := $(ABS_SYSROOT)/usr/local

SRC_DIR   := openocd
BUILD_DIR := build/openocd

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------

.PHONY: all openocd clean distclean

all: openocd

openocd: $(PREFIX)/bin/openocd

$(PREFIX)/bin/openocd:
	@echo "==> Building OpenOCD $(OPENOCD_VERSION)"
	mkdir -p $(BUILD_DIR)
	if [ ! -d $(SRC_DIR) ]; then \
		git clone $(OPENOCD_REPO) $(SRC_DIR); \
	fi
	cd $(SRC_DIR) && git fetch && git checkout $(OPENOCD_VERSION)
	cd $(SRC_DIR) && ./bootstrap
	cd $(SRC_DIR) && ./configure \
		--prefix=$(PREFIX) \
		--enable-stlink \
		--enable-cmsis-dap \
		--disable-werror
	cd $(SRC_DIR) && $(MAKE) -j$$(nproc)
	cd $(SRC_DIR) && $(MAKE) install

clean:
	cd $(SRC_DIR) && $(MAKE) clean || true

distclean:
	rm -rf $(SRC_DIR) $(BUILD_DIR)


VERSION   := 15.2.rel1
PLATFORM  := x86_64
ARCHIVE   := arm-gnu-toolchain-$(VERSION)-$(PLATFORM)-arm-none-eabi.tar.xz
BASE_URL  := https://developer.arm.com/-/media/Files/downloads/gnu/$(VERSION)/binrel
URL       := $(BASE_URL)/$(ARCHIVE)
DESTDIR   := gcc-arm-none-eabi

.PHONY: all

all: stm32-gcc

stm32-gcc: .fetch-stm32-gcc .unpack-stm32-gcc

.fetch-stm32-gcc:
	@echo "Fetching ARM GCC $(ARCHIVE)"
	wget -q $(URL) && touch $@

.unpack-stm32-gcc:
	@echo "Unpacking ARM GCC $(ARCHIVE)"
	mkdir -p $(DESTDIR)
	tar -xvf $(ARCHIVE) -C $(DESTDIR) --strip-components=1 && touch $@

clean:
	rm -rf .fetch-stm32-gcc .unpack-stm32-gcc $(ARCHIVE)


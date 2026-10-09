# LiteOS root Makefile: orquesta kernel, libc, sysroot, ISO y QEMU.

include toolchain/toolchain.mk
.DEFAULT_GOAL := all

BUILD  := build
KERNEL := $(BUILD)/kernel.elf
TOOLCHAIN_CONFIG := $(BUILD)/.toolchain-config

SRCS   := $(shell find src -path src/user -prune -o -name '*.c' -print) \
          $(shell find src -path src/user -prune -o -name '*.S' -print)
OBJS   := $(patsubst %.c,$(BUILD)/%.o,$(filter %.c,$(SRCS))) \
          $(patsubst %.S,$(BUILD)/%.o,$(filter %.S,$(SRCS)))

LIBC_SRCS := $(wildcard libc/src/*.c)
LIBC_OBJS := $(patsubst libc/src/%.c,$(BUILD)/libc/%.o,$(LIBC_SRCS))

LIBC_KERN_OBJS := $(patsubst libc/src/%.c,$(BUILD)/libc_kern/%.o,$(LIBC_SRCS))
USER_CFLAGS := $(CFLAGS) -fno-pie -Ilibc/include -Isrc/user/sh
# Mantener el ELF de usuario fuera del mapeo identidad supervisor del kernel.
USER_LDFLAGS := -nostdlib -static -no-pie -Wl,--build-id=none,-Ttext=0x10000001000
USER_BINS := $(BUILD)/user/bin/sh $(BUILD)/user/bin/hello
USER_OBJS := $(BUILD)/userland/start.o $(BUILD)/userland/sh/main.o \
             $(BUILD)/userland/sh/parser.o $(BUILD)/userland/hello.o
DEPS := $(OBJS:.o=.d) $(LIBC_OBJS:.o=.d) $(LIBC_KERN_OBJS:.o=.d) $(USER_OBJS:.o=.d)

-include $(DEPS)

.PHONY: all kernel libc sysroot iso run debug test test-crypto test-libc test-toolchain ci clean toolchain-check FORCE
HOST_CC ?= gcc

all kernel libc sysroot iso run debug test: | toolchain-check

ci: all
	@echo "[ci] LiteOS build pipeline completed successfully"

toolchain-check:
	@./scripts/check-toolchain.sh "$(TARGET)" "$(TOOLCHAIN_MODE)" "$(CC)" "$(AS)" "$(LD)" "$(AR)" "$(OBJCOPY)" "$(STRIP)" "$(TOOLCHAIN_ROOT)"

$(TOOLCHAIN_CONFIG): FORCE | toolchain-check
	@mkdir -p $(BUILD)
	@printf '%s\n' '$(TARGET)' '$(TOOLCHAIN_MODE)' '$(CC)' '$(AS)' '$(LD)' '$(AR)' '$(OBJCOPY)' '$(STRIP)' '$(CFLAGS)' '$(USER_CFLAGS)' '$(USER_LDFLAGS)' '$(LDFLAGS)' > $@.tmp
	@if cmp -s $@.tmp $@; then rm $@.tmp; else mv $@.tmp $@; fi

all: kernel libc sysroot iso

$(BUILD)/%.o: %.c $(TOOLCHAIN_CONFIG) | toolchain-check
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -Isrc -Ilibc/include -c $< -o $@

$(BUILD)/%.o: %.S $(TOOLCHAIN_CONFIG) | toolchain-check
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -Isrc -Ilibc/include -c $< -o $@

kernel: $(KERNEL)

$(KERNEL): $(OBJS) $(LIBC_KERN_OBJS) $(BUILD)/initramfs.o linker.ld
	@mkdir -p $(BUILD)
	$(LD) $(LDFLAGS) -T linker.ld $(OBJS) $(LIBC_KERN_OBJS) $(BUILD)/initramfs.o -o $@

$(BUILD)/userland/%.o: src/user/%.c $(TOOLCHAIN_CONFIG) | toolchain-check
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/userland/%.o: src/user/%.S $(TOOLCHAIN_CONFIG) | toolchain-check
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/user/bin/sh: $(BUILD)/userland/start.o $(BUILD)/userland/sh/main.o $(BUILD)/userland/sh/parser.o $(BUILD)/libliteosc.a
	@mkdir -p $(dir $@)
	$(CC) $(USER_LDFLAGS) $^ -o $@
	$(STRIP) --strip-debug $@

$(BUILD)/user/bin/hello: $(BUILD)/userland/start.o $(BUILD)/userland/hello.o $(BUILD)/libliteosc.a
	@mkdir -p $(dir $@)
	$(CC) $(USER_LDFLAGS) $^ -o $@
	$(STRIP) --strip-debug $@

$(BUILD)/initramfs.cpio: $(USER_BINS) scripts/mkinitramfs.py
	python3 scripts/mkinitramfs.py $@ $(USER_BINS)

$(BUILD)/initramfs.o: $(BUILD)/initramfs.cpio
	cd $(BUILD) && $(OBJCOPY) -I binary -O elf64-x86-64 -B i386:x86-64 initramfs.cpio initramfs.o
	$(OBJCOPY) --rename-section .data=.rodata,alloc,load,readonly,data,contents $@

$(BUILD)/libc_kern/%.o: libc/src/%.c $(TOOLCHAIN_CONFIG) | toolchain-check
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -Ilibc/include -c $< -o $@

libc: $(BUILD)/libliteosc.a

$(BUILD)/libc/%.o: libc/src/%.c $(TOOLCHAIN_CONFIG) | toolchain-check
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -Ilibc/include -c $< -o $@

$(BUILD)/libliteosc.a: $(LIBC_OBJS)
	$(AR) rcs $@ $^

sysroot: libc $(USER_BINS)
	mkdir -p sysroot/lib sysroot/usr/include/sys sysroot/bin
	cp $(BUILD)/libliteosc.a sysroot/lib/
	cp libc/include/*.h sysroot/usr/include/
	cp libc/include/sys/*.h sysroot/usr/include/sys/
	cp $(USER_BINS) sysroot/bin/

iso: kernel grub.cfg
	rm -rf $(BUILD)/iso
	mkdir -p $(BUILD)/iso/boot/grub
	cp $(KERNEL) $(BUILD)/iso/boot/kernel.elf
	cp grub.cfg $(BUILD)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD)/liteos.iso $(BUILD)/iso 2>/dev/null

run: iso
	qemu-system-x86_64 -cdrom $(BUILD)/liteos.iso -serial stdio -no-reboot \
	    -device isa-debug-exit,iobase=0xf4,iosize=0x04

debug: iso
	qemu-system-x86_64 -cdrom $(BUILD)/liteos.iso -serial stdio -s -S -no-reboot \
	    -device isa-debug-exit,iobase=0xf4,iosize=0x04

$(BUILD)/parser_test: tests/parser_test.c src/user/sh/parser.c src/user/sh/parser.h
	@mkdir -p $(BUILD)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Isrc/user/sh tests/parser_test.c src/user/sh/parser.c -o $@

$(BUILD)/stdio_test: tests/stdio_test.c libc/src/stdio.c libc/include/stdio.h libc/include/stdarg.h
	@mkdir -p $(BUILD)
	$(HOST_CC) -std=gnu11 -ffreestanding -Wall -Wextra -Werror -nostdinc -Ilibc/include -c libc/src/stdio.c -o $(BUILD)/stdio_test_stdio.o
	$(HOST_CC) -std=gnu11 -Wall -Wextra -Werror -fno-builtin-snprintf tests/stdio_test.c $(BUILD)/stdio_test_stdio.o -o $@

$(BUILD)/crypto_test: tests/crypto_test.c src/crypto/sha256.c src/crypto/sha512.c src/crypto.h src/types.h
	@mkdir -p $(BUILD)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Isrc tests/crypto_test.c src/crypto/sha256.c src/crypto/sha512.c -o $@

test-crypto: $(BUILD)/crypto_test
	@$(BUILD)/crypto_test

test-libc: $(BUILD)/stdio_test
	@$(BUILD)/stdio_test

test-toolchain:
	@./tests/toolchain_test.sh

test: iso $(BUILD)/parser_test test-libc test-toolchain test-crypto
	@$(BUILD)/parser_test
	@python3 tests/initramfs_test.py $(BUILD)/initramfs.cpio
	@python3 tests/boot_test.py $(BUILD)/liteos.iso

clean:
	rm -rf $(BUILD)

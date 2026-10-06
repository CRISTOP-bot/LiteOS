# LiteOS root Makefile: orquesta kernel, libc, sysroot, ISO y QEMU.

include toolchain/toolchain.mk

BUILD  := build
KERNEL := $(BUILD)/kernel.elf

SRCS   := $(shell find src -name '*.c') $(shell find src -name '*.S')
OBJS   := $(patsubst %.c,$(BUILD)/%.o,$(filter %.c,$(SRCS))) \
          $(patsubst %.S,$(BUILD)/%.o,$(filter %.S,$(SRCS)))

LIBC_SRCS := $(wildcard libc/src/*.c)
LIBC_OBJS := $(patsubst libc/src/%.c,$(BUILD)/libc/%.o,$(LIBC_SRCS))

LIBC_KERN_OBJS := $(patsubst libc/src/%.c,$(BUILD)/libc_kern/%.o,$(LIBC_SRCS))

.PHONY: all kernel libc sysroot iso run debug test clean

all: kernel libc sysroot iso

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Isrc -Ilibc/include -c $< -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Isrc -Ilibc/include -c $< -o $@

kernel: $(KERNEL)

$(KERNEL): $(OBJS) $(LIBC_KERN_OBJS) linker.ld
	@mkdir -p $(BUILD)
	$(LD) $(LDFLAGS) -T linker.ld $(OBJS) $(LIBC_KERN_OBJS) -o $@

$(BUILD)/libc_kern/%.o: libc/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Ilibc/include -c $< -o $@

libc: $(BUILD)/libliteosc.a

$(BUILD)/libc/%.o: libc/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Ilibc/include -c $< -o $@

$(BUILD)/libliteosc.a: $(LIBC_OBJS)
	$(AR) rcs $@ $^

sysroot: libc
	mkdir -p sysroot/lib sysroot/usr/include
	cp $(BUILD)/libliteosc.a sysroot/lib/
	cp libc/include/*.h sysroot/usr/include/

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

test: iso
	@./tests/boot_test.sh $(BUILD)/liteos.iso

clean:
	rm -rf $(BUILD)

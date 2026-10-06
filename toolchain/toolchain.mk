# LiteOS toolchain configuration.
#
# Si existe un cross-compiler x86_64-elf lo usa; si no, se usa el GCC del
# host en modo freestanding. Para instalar una toolchain real:
#   scripts/setup-toolchain.sh
# y exportar TOOLCHAIN_ROOT.

TARGET        ?= x86_64-elf
TOOLCHAIN_ROOT ?= /usr

ifneq ($(wildcard $(TOOLCHAIN_ROOT)/bin/$(TARGET)-gcc),)
CC      := $(TOOLCHAIN_ROOT)/bin/$(TARGET)-gcc
AS      := $(TOOLCHAIN_ROOT)/bin/$(TARGET)-as
LD      := $(TOOLCHAIN_ROOT)/bin/$(TARGET)-ld
AR      := $(TOOLCHAIN_ROOT)/bin/$(TARGET)-ar
OBJCOPY := $(TOOLCHAIN_ROOT)/bin/$(TARGET)-objcopy
OBJDUMP := $(TOOLCHAIN_ROOT)/bin/$(TARGET)-objdump
else
CC      := gcc
AS      := as
LD      := ld
AR      := ar
OBJCOPY := objcopy
OBJDUMP := objdump
endif

CFLAGS  ?= -std=gnu11 -ffreestanding -fno-stack-protector -fno-pic \
           -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mgeneral-regs-only \
           -mcmodel=large -Wall -Wextra -Werror -O2 -g
ASFLAGS ?= -64
LDFLAGS ?= -nostdlib -static

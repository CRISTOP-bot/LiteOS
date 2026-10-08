# Cross-toolchain by default. Native freestanding builds require explicit opt-in.
TARGET ?= x86_64-elf
TOOLCHAIN_MODE ?= cross
TOOLCHAIN_ROOT ?=

ifeq ($(filter $(TOOLCHAIN_MODE),cross host),)
$(error TOOLCHAIN_MODE debe ser cross o host (recibido: $(TOOLCHAIN_MODE)))
endif

ifeq ($(TOOLCHAIN_MODE),cross)
ifneq ($(strip $(TOOLCHAIN_ROOT)),)
TOOL_PREFIX := $(patsubst %/,%,$(TOOLCHAIN_ROOT))/bin/$(TARGET)-
else
TOOL_PREFIX := $(TARGET)-
endif
else
ifneq ($(strip $(TOOLCHAIN_ROOT)),)
$(error TOOLCHAIN_ROOT solo se usa con TOOLCHAIN_MODE=cross)
endif
TOOL_PREFIX :=
endif

# Respect command-line and environment overrides, but not Make's built-in cc/as/ld.
define set_tool
ifeq ($(origin $(1)),default)
$(1) := $(TOOL_PREFIX)$(2)
endif
ifeq ($(origin $(1)),undefined)
$(1) := $(TOOL_PREFIX)$(2)
endif
endef

$(eval $(call set_tool,CC,gcc))
$(eval $(call set_tool,AS,as))
$(eval $(call set_tool,LD,ld))
$(eval $(call set_tool,AR,ar))
$(eval $(call set_tool,OBJCOPY,objcopy))
$(eval $(call set_tool,STRIP,strip))

CFLAGS ?= -std=gnu11 -ffreestanding -nostdinc -fno-stack-protector -fno-pic -fno-pie \
          -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mgeneral-regs-only \
          -mcmodel=large -Wall -Wextra -Werror -O2 -g
LDFLAGS ?= -nostdlib -static

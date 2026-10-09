# App build rules for LiteOS

BUSYBOX_VERSION ?= 1.36.1
BUSYBOX_URL ?= https://busybox.net/downloads/busybox-$(BUSYBOX_VERSION).tar.bz2
BUSYBOX_SRC := $(APPS_DIR)/busybox/src/busybox-$(BUSYBOX_VERSION)
BUSYBOX_BUILD := $(BUILD_DIR)/busybox
BUSYBOX_INSTALL := $(ROOTFS_DIR)/busybox-install

LUA_VERSION ?= 5.4.6
LUA_URL ?= https://www.lua.org/ftp/lua-$(LUA_VERSION).tar.gz
LUA_SRC := $(APPS_DIR)/lua/src/lua-$(LUA_VERSION)
LUA_BUILD := $(BUILD_DIR)/lua
LUA_INSTALL := $(ROOTFS_DIR)/lua-install

TCC_VERSION ?= 0.9.27
TCC_URL ?= https://download.savannah.gnu.org/releases/tinycc/tcc-$(TCC_VERSION).tar.bz2
TCC_SRC := $(APPS_DIR)/tcc/src/tcc-$(TCC_VERSION)
TCC_BUILD := $(BUILD_DIR)/tcc
TCC_INSTALL := $(ROOTFS_DIR)/tcc-install

NANO_VERSION ?= 7.2
NANO_URL ?= https://www.nano-editor.org/dist/v$(NANO_VERSION)/nano-$(NANO_VERSION).tar.xz
NANO_SRC := $(APPS_DIR)/nano/src/nano-$(NANO_VERSION)
NANO_BUILD := $(BUILD_DIR)/nano
NANO_INSTALL := $(ROOTFS_DIR)/nano-install

busybox: $(BUSYBOX_INSTALL)/bin/busybox

$(BUSYBOX_SRC):
	@mkdir -p $(APPS_DIR)/busybox/src
	@cd $(APPS_DIR)/busybox/src && wget -q $(BUSYBOX_URL) -O busybox-$(BUSYBOX_VERSION).tar.bz2 && tar -xjf busybox-$(BUSYBOX_VERSION).tar.bz2

$(BUSYBOX_INSTALL)/bin/busybox: $(BUSYBOX_SRC)
	@mkdir -p $(BUSYBOX_BUILD) $(BUSYBOX_INSTALL)
	@cd $(BUSYBOX_SRC) && make distclean >/dev/null 2>&1 || true
	@cd $(BUSYBOX_SRC) && make defconfig
	@cd $(BUSYBOX_SRC) && make CROSS_COMPILE=$(TARGET)- CONFIG_PREFIX=$(BUSYBOX_INSTALL) -j$$(nproc)
	@cd $(BUSYBOX_SRC) && make CROSS_COMPILE=$(TARGET)- CONFIG_PREFIX=$(BUSYBOX_INSTALL) install
	@echo "[+] BusyBox built"

lua: $(LUA_INSTALL)/lib/liblua.a

$(LUA_SRC):
	@mkdir -p $(APPS_DIR)/lua/src
	@cd $(APPS_DIR)/lua/src && wget -q $(LUA_URL) -O lua-$(LUA_VERSION).tar.gz && tar -xzf lua-$(LUA_VERSION).tar.gz

$(LUA_INSTALL)/lib/liblua.a: $(LUA_SRC)
	@mkdir -p $(LUA_BUILD) $(LUA_INSTALL)
	@cd $(LUA_SRC) && make CC="$(CC)" AR="$(AR)" RANLIB="$(RANLIB)" linux -j$$(nproc)
	@cd $(LUA_SRC) && make INSTALL_TOP=$(LUA_INSTALL) install
	@echo "[+] Lua built"

tcc: $(TCC_INSTALL)/bin/tcc

$(TCC_SRC):
	@mkdir -p $(APPS_DIR)/tcc/src
	@cd $(APPS_DIR)/tcc/src && wget -q $(TCC_URL) -O tcc-$(TCC_VERSION).tar.bz2 && tar -xjf tcc-$(TCC_VERSION).tar.bz2

$(TCC_INSTALL)/bin/tcc: $(TCC_SRC)
	@mkdir -p $(TCC_BUILD) $(TCC_INSTALL)
	@cd $(TCC_SRC) && ./configure --cc="$(CC)" --prefix=$(TCC_INSTALL) --cross-prefix=$(TARGET)-
	@cd $(TCC_SRC) && make -j$$(nproc)
	@cd $(TCC_SRC) && make install
	@echo "[+] TCC built"

nano: $(NANO_INSTALL)/bin/nano

$(NANO_SRC):
	@mkdir -p $(APPS_DIR)/nano/src
	@cd $(APPS_DIR)/nano/src && wget -q $(NANO_URL) -O nano-$(NANO_VERSION).tar.xz && tar -xJf nano-$(NANO_VERSION).tar.xz

$(NANO_INSTALL)/bin/nano: $(NANO_SRC)
	@mkdir -p $(NANO_BUILD) $(NANO_INSTALL)
	@cd $(NANO_SRC) && CC="$(CC)" ./configure --prefix=$(NANO_INSTALL) --host=$(TARGET)
	@cd $(NANO_SRC) && make -j$$(nproc)
	@cd $(NANO_SRC) && make install
	@echo "[+] Nano built"

.PHONY: busybox lua tcc nano



























































































































































































# HIPS Kernel Module Makefile
# 支持 CentOS 8/9 和 Ubuntu 22.04/24.04

obj-m := hips.o

# 内核版本检测
KERNEL_VERSION := $(shell uname -r)
KERNEL_MAJOR := $(shell echo $(KERNEL_VERSION) | cut -d. -f1)
KERNEL_MINOR := $(shell echo $(KERNEL_VERSION) | cut -d. -f2)

# 编译标志
ccflags-y := -DDEBUG -DCONFIG_HIPS_DEBUG

# 根据内核版本设置不同的编译标志
ifeq ($(shell test $(KERNEL_MAJOR) -ge 5; echo $$?), 0)
    ccflags-y += -DKERNEL_5_PLUS
endif

ifeq ($(shell test $(KERNEL_MAJOR) -ge 6; echo $$?), 0)
    ccflags-y += -DKERNEL_6_PLUS
endif

# 默认目标
all: module tools

# 编译内核模块
module:
	$(MAKE) -C /lib/modules/$(KERNEL_VERSION)/build M=$(PWD) modules

# 编译用户空间工具
tools:
	$(CC) -o hipsctl tools/hipsctl.c -Iinclude
	$(CC) -o hips-config tools/hips-config.c -Iinclude

# 安装模块
install: module
	$(MAKE) -C /lib/modules/$(KERNEL_VERSION)/build M=$(PWD) modules_install
	depmod -a

# 卸载模块
uninstall:
	rmmod hips || true
	rm -f /lib/modules/$(KERNEL_VERSION)/extra/hips.ko

# 清理
clean:
	$(MAKE) -C /lib/modules/$(KERNEL_VERSION)/build M=$(PWD) clean
	rm -f hipsctl hips-config

# 加载模块
load: module
	insmod hips.ko

# 卸载模块
unload:
	rmmod hips

# 重新加载模块
reload: unload load

# 检查内核配置
check-config:
	@echo "检查内核配置..."
	@if [ ! -f /proc/config.gz ]; then \
		echo "警告: 无法读取内核配置"; \
	else \
		zcat /proc/config.gz | grep -E "(CONFIG_SECURITY|CONFIG_NETFILTER)" || echo "警告: 缺少必要的内核配置"; \
	fi

.PHONY: all module tools install uninstall clean load unload reload check-config 
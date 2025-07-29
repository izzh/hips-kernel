# HIPS - 主机入侵防御系统

面向企业的Linux系统上的轻量级主机入侵防御系统(HIPS)。根据威胁情报或者用户自定义配置阻止已知（恶意）软件的执行，阻止（恶意）域名解析，阻止到（恶意）ip 的网络行为。从而阻止破坏系统和阻止数据泄露。

## 功能特性

1. **进程执行拦截** - 阻止恶意程序（病毒、木马等）和危险程序在系统上执行
2. **DNS解析拦截** - 阻止恶意程序解析(恶意)域名，阻止对特定的（恶意）域名解析
3. **网络连接拦截** - 阻止恶意程序发起网络连接、收发数据，阻止向特定的（恶意）地址（ipv4和ipv6)发起网络连接、收发数据
4. **实时监控** - 提供实时事件监控和日志记录
5. **灵活配置** - 支持动态规则配置和热重载
6. **多平台支持** - 兼容CentOS 8/9和Ubuntu 22.04/24.04

## 系统要求

- Linux内核版本：4.18+ (CentOS 8), 5.14+ (CentOS 9), 5.15+ (Ubuntu 22.04), 6.8+ (Ubuntu 24.04)
- 内核头文件：`kernel-devel` 或 `linux-headers`
- 编译工具：`gcc`, `make`
- 权限：需要root权限加载内核模块

## 安装说明

### 1. 准备环境

```bash
# CentOS/RHEL
sudo yum install kernel-devel gcc make

# Ubuntu/Debian
sudo apt-get install linux-headers-$(uname -r) gcc make
```

### 2. 编译模块

```bash
cd hips-kernel
make clean
make all
```

### 3. 安装模块

```bash
# 安装内核模块
sudo make install

# 创建设备节点
sudo mknod /dev/hips c 240 0
sudo chmod 666 /dev/hips

# 加载模块
sudo modprobe hips
```

### 4. 验证安装

```bash
# 检查模块是否加载
lsmod | grep hips

# 检查设备文件
ls -la /dev/hips

# 检查proc接口
ls -la /proc/hips/
```

## 使用方法

### 基本命令

```bash
# 显示模块状态
sudo ./hipsctl status

# 启用模块
sudo ./hipsctl enable

# 禁用模块
sudo ./hipsctl disable

# 显示统计信息
sudo ./hipsctl stats

# 显示日志记录
sudo ./hipsctl logs

# 重新加载配置
sudo ./hipsctl reload
```

### 规则管理

```bash
# 添加执行规则
sudo ./hipsctl add-rule exec block 100 /usr/bin/malware.exe "恶意软件"

# 添加DNS规则
sudo ./hipsctl add-rule dns block 50 evil.com "恶意域名"

# 添加网络规则
sudo ./hipsctl add-rule network block 75 192.168.1.100 "恶意IP"

# 删除规则
sudo ./hipsctl del-rule 1
```

### 配置文件

模块支持通过配置文件进行批量规则配置：

```bash
# 编辑配置文件
sudo vi /etc/hips/config.json

# 重新加载配置
sudo ./hipsctl reload
```

配置文件格式示例：

```json
{
  "enabled": true,
  "log_level": 2,
  "max_rules": 1000,
  "rules": [
    {
      "type": "exec",
      "action": "block",
      "priority": 100,
      "target": "/usr/bin/malware.exe",
      "description": "恶意软件"
    },
    {
      "type": "dns",
      "action": "block",
      "priority": 50,
      "target": "evil.com",
      "description": "恶意域名"
    },
    {
      "type": "network",
      "action": "block",
      "priority": 75,
      "target": "192.168.1.100",
      "description": "恶意IP"
    }
  ]
}
```

### Proc接口

模块提供以下proc接口：

```bash
# 查看状态
cat /proc/hips/status

# 查看规则
cat /proc/hips/rules

# 查看日志
cat /proc/hips/logs

# 添加规则（通过写入）
echo "exec|block|100|/usr/bin/malware.exe|恶意软件" > /proc/hips/rules
```

## 规则类型

### 1. 执行规则 (exec)

阻止或记录特定程序的执行：

- **目标格式**: 文件路径（支持通配符）
- **示例**: `/usr/bin/malware.exe`, `/tmp/*.exe`

### 2. DNS规则 (dns)

阻止或记录特定域名的解析：

- **目标格式**: 域名（支持通配符）
- **示例**: `evil.com`, `*.malware.com`

### 3. 网络规则 (network)

阻止或记录特定IP地址的网络连接：

- **目标格式**: IP地址或CIDR
- **示例**: `192.168.1.100`, `10.0.0.0/8`

## 动作类型

- **block**: 阻止操作
- **allow**: 允许操作
- **log**: 仅记录操作

## 优先级

规则优先级为0-999，数字越大优先级越高。当多个规则匹配时，优先级高的规则生效。

## 日志和监控

### 日志级别

- **0 (ERROR)**: 仅记录错误
- **1 (WARN)**: 记录警告和错误
- **2 (INFO)**: 记录信息、警告和错误
- **3 (DEBUG)**: 记录所有信息

### 日志查看

```bash
# 查看内核日志
dmesg | grep HIPS

# 查看系统日志
journalctl | grep HIPS

# 查看模块日志
cat /proc/hips/logs
```

## 故障排除

### 常见问题

1. **模块加载失败**
   ```bash
   # 检查内核版本兼容性
   uname -r
   
   # 检查内核头文件
   ls /usr/src/kernels/
   
   # 重新编译
   make clean && make all
   ```

2. **权限问题**
   ```bash
   # 检查设备文件权限
   ls -la /dev/hips
   
   # 重新创建设备文件
   sudo mknod /dev/hips c 240 0
   sudo chmod 666 /dev/hips
   ```

3. **规则不生效**
   ```bash
   # 检查模块状态
   sudo ./hipsctl status
   
   # 检查规则列表
   cat /proc/hips/rules
   
   # 重新加载配置
   sudo ./hipsctl reload
   ```

### 调试模式

启用调试模式获取详细日志：

```bash
# 加载模块时启用调试
sudo modprobe hips hips_log_level=3

# 或通过proc接口修改
echo 3 > /proc/hips/config
```

## 性能考虑

- 规则数量影响性能，建议控制在1000条以内
- 网络规则对性能影响较大，建议谨慎使用
- 日志级别越高，性能开销越大

## 安全注意事项

1. **权限控制**: 只有root用户才能加载模块和修改配置
2. **规则验证**: 添加规则前请仔细验证，避免误杀正常程序
3. **备份配置**: 修改配置前请备份原有配置
4. **测试环境**: 建议在测试环境验证规则后再部署到生产环境

## 开发说明

### 项目结构

```
hips-kernel/
├── Makefile              # 构建脚本
├── README.md            # 项目文档
├── include/
│   └── hips.h           # 用户空间头文件
├── src/
│   ├── hips_common.h    # 内核公共头文件
│   ├── hips_main.c      # 主模块
│   ├── hips_hooks.c     # 安全钩子
│   ├── hips_config.c    # 配置管理
│   └── hips_procfs.c    # Proc接口
└── tools/
    └── hipsctl.c        # 控制工具
```

### 编译选项

```bash
# 启用调试
make DEBUG=1

# 指定内核版本
make KERNEL_VERSION=5.15.0

# 交叉编译
make ARCH=x86_64 CROSS_COMPILE=x86_64-linux-gnu-
```

### 扩展开发

如需添加新的规则类型或功能，请参考现有代码结构：

1. 在 `include/hips.h` 中定义新的数据结构
2. 在 `src/hips_hooks.c` 中实现新的钩子函数
3. 在 `src/hips_config.c` 中添加规则处理逻辑
4. 在 `tools/hipsctl.c` 中添加用户空间接口

## 许可证

本项目采用 GPL v2 许可证。

## 贡献

欢迎提交Issue和Pull Request来改进项目。

## 联系方式

如有问题或建议，请通过以下方式联系：

- 提交Issue
- 发送邮件至项目维护者

---

**注意**: 本软件仅供学习和研究使用，在生产环境使用前请充分测试。 
#!/bin/bash

# HIPS 内核模块构建脚本
# 支持 CentOS 8/9 和 Ubuntu 22.04/24.04

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查是否为root用户
check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "此脚本需要root权限运行"
        exit 1
    fi
}

# 检查系统要求
check_requirements() {
    print_info "检查系统要求..."
    
    # 检查内核版本
    KERNEL_VERSION=$(uname -r)
    print_info "内核版本: $KERNEL_VERSION"
    
    # 检查必要的包
    if command -v yum &> /dev/null; then
        # CentOS/RHEL
        if ! rpm -q kernel-devel &> /dev/null; then
            print_error "缺少 kernel-devel 包，请运行: yum install kernel-devel"
            exit 1
        fi
    elif command -v apt-get &> /dev/null; then
        # Ubuntu/Debian
        if ! dpkg -l | grep -q linux-headers; then
            print_error "缺少 linux-headers 包，请运行: apt-get install linux-headers-\$(uname -r)"
            exit 1
        fi
    else
        print_error "不支持的包管理器"
        exit 1
    fi
    
    # 检查编译工具
    if ! command -v gcc &> /dev/null; then
        print_error "缺少 gcc 编译器"
        exit 1
    fi
    
    if ! command -v make &> /dev/null; then
        print_error "缺少 make 工具"
        exit 1
    fi
    
    print_success "系统要求检查通过"
}

# 清理构建
clean_build() {
    print_info "清理构建文件..."
    make clean
    rm -f hipsctl hips-config
    print_success "清理完成"
}

# 编译模块
compile_module() {
    print_info "编译内核模块..."
    
    # 检查内核头文件路径
    KERNEL_HEADERS=""
    if [ -d "/usr/src/kernels/$(uname -r)" ]; then
        KERNEL_HEADERS="/usr/src/kernels/$(uname -r)"
    elif [ -d "/usr/src/linux-headers-$(uname -r)" ]; then
        KERNEL_HEADERS="/usr/src/linux-headers-$(uname -r)"
    else
        print_error "找不到内核头文件"
        exit 1
    fi
    
    print_info "使用内核头文件: $KERNEL_HEADERS"
    
    # 编译模块
    if make all; then
        print_success "模块编译成功"
    else
        print_error "模块编译失败"
        exit 1
    fi
}

# 安装模块
install_module() {
    print_info "安装内核模块..."
    
    # 卸载旧模块（如果存在）
    if lsmod | grep -q hips; then
        print_info "卸载旧模块..."
        modprobe -r hips 2>/dev/null || true
    fi
    
    # 安装模块
    if make install; then
        print_success "模块安装成功"
    else
        print_error "模块安装失败"
        exit 1
    fi
    
    # 创建设备文件
    if [ ! -e "/dev/hips" ]; then
        print_info "创建设备文件..."
        mknod /dev/hips c 240 0 2>/dev/null || true
        chmod 666 /dev/hips
    fi
    
    # 加载模块
    print_info "加载模块..."
    if modprobe hips; then
        print_success "模块加载成功"
    else
        print_error "模块加载失败"
        exit 1
    fi
}

# 验证安装
verify_installation() {
    print_info "验证安装..."
    
    # 检查模块是否加载
    if lsmod | grep -q hips; then
        print_success "模块已加载"
    else
        print_error "模块未加载"
        exit 1
    fi
    
    # 检查设备文件
    if [ -e "/dev/hips" ]; then
        print_success "设备文件存在"
    else
        print_error "设备文件不存在"
        exit 1
    fi
    
    # 检查proc接口
    if [ -d "/proc/hips" ]; then
        print_success "Proc接口可用"
    else
        print_error "Proc接口不可用"
        exit 1
    fi
    
    print_success "安装验证通过"
}

# 创建配置文件
create_config() {
    print_info "创建配置文件..."
    
    mkdir -p /etc/hips
    
    if [ ! -f "/etc/hips/config.json" ]; then
        cat > /etc/hips/config.json << 'EOF'
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
      "description": "示例恶意软件"
    },
    {
      "type": "dns",
      "action": "block",
      "priority": 50,
      "target": "evil.com",
      "description": "示例恶意域名"
    },
    {
      "type": "network",
      "action": "block",
      "priority": 75,
      "target": "192.168.1.100",
      "description": "示例恶意IP"
    }
  ]
}
EOF
        print_success "配置文件创建完成"
    else
        print_info "配置文件已存在"
    fi
}

# 显示使用说明
show_usage() {
    print_info "HIPS 安装完成！"
    echo
    echo "使用方法："
    echo "  查看状态:    ./hipsctl status"
    echo "  启用模块:    ./hipsctl enable"
    echo "  禁用模块:    ./hipsctl disable"
    echo "  查看统计:    ./hipsctl stats"
    echo "  查看日志:    ./hipsctl logs"
    echo "  添加规则:    ./hipsctl add-rule exec block 100 /path/to/malware '恶意软件'"
    echo "  删除规则:    ./hipsctl del-rule 1"
    echo "  重新加载:    ./hipsctl reload"
    echo
    echo "Proc接口："
    echo "  查看状态:    cat /proc/hips/status"
    echo "  查看规则:    cat /proc/hips/rules"
    echo "  查看日志:    cat /proc/hips/logs"
    echo
    echo "配置文件："
    echo "  编辑配置:    vi /etc/hips/config.json"
    echo "  重新加载:    ./hipsctl reload"
}

# 卸载模块
uninstall_module() {
    print_info "卸载HIPS模块..."
    
    # 卸载模块
    if lsmod | grep -q hips; then
        modprobe -r hips
        print_success "模块已卸载"
    fi
    
    # 删除设备文件
    if [ -e "/dev/hips" ]; then
        rm -f /dev/hips
        print_success "设备文件已删除"
    fi
    
    # 删除配置文件
    if [ -d "/etc/hips" ]; then
        rm -rf /etc/hips
        print_success "配置文件已删除"
    fi
    
    print_success "卸载完成"
}

# 主函数
main() {
    case "${1:-install}" in
        "install")
            check_root
            check_requirements
            clean_build
            compile_module
            install_module
            create_config
            verify_installation
            show_usage
            ;;
        "uninstall")
            check_root
            uninstall_module
            ;;
        "clean")
            clean_build
            ;;
        "compile")
            check_requirements
            clean_build
            compile_module
            ;;
        "help"|"-h"|"--help")
            echo "HIPS 内核模块构建脚本"
            echo
            echo "用法: $0 [命令]"
            echo
            echo "命令:"
            echo "  install    安装模块（默认）"
            echo "  uninstall  卸载模块"
            echo "  clean      清理构建文件"
            echo "  compile    仅编译模块"
            echo "  help       显示此帮助信息"
            ;;
        *)
            print_error "未知命令: $1"
            echo "使用 '$0 help' 查看帮助信息"
            exit 1
            ;;
    esac
}

# 运行主函数
main "$@" 
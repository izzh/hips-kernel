#!/bin/bash

# HIPS SSH阻断功能测试脚本
# 测试特定IP阻断SSH连接的功能

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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
        echo "请使用: sudo $0"
        exit 1
    fi
}

# 检查HIPS模块是否加载
check_hips_module() {
    if ! lsmod | grep -q hips; then
        print_error "HIPS模块未加载"
        echo "请先运行: sudo ./build.sh install"
        exit 1
    fi
    print_success "HIPS模块已加载"
}

# 检查SSH服务状态
check_ssh_service() {
    if systemctl is-active --quiet sshd; then
        print_success "SSH服务正在运行"
    else
        print_warning "SSH服务未运行，正在启动..."
        systemctl start sshd
        sleep 2
        if systemctl is-active --quiet sshd; then
            print_success "SSH服务已启动"
        else
            print_error "无法启动SSH服务"
            exit 1
        fi
    fi
}

# 获取本机IP地址
get_local_ip() {
    local_ip=$(ip route get 8.8.8.8 | awk '{print $7; exit}')
    echo "$local_ip"
}

# 添加SSH阻断规则
add_ssh_block_rule() {
    local target_ip="$1"
    local rule_desc="$2"
    
    print_info "添加SSH阻断规则: $target_ip -> SSH (端口22)"
    
    # 使用hipsctl添加规则
    if ./tools/hipsctl add-rule network block 100 "$target_ip:22" "$rule_desc"; then
        print_success "SSH阻断规则添加成功"
    else
        print_error "SSH阻断规则添加失败"
        return 1
    fi
}

# 测试SSH连接
test_ssh_connection() {
    local target_ip="$1"
    local expected_result="$2"  # "block" 或 "allow"
    
    print_info "测试SSH连接到 $target_ip..."
    
    # 尝试SSH连接，设置超时
    if timeout 5 ssh -o ConnectTimeout=3 -o StrictHostKeyChecking=no root@"$target_ip" "echo 'SSH连接测试'" 2>/dev/null; then
        if [[ "$expected_result" == "block" ]]; then
            print_error "SSH连接应该被阻断，但连接成功"
            return 1
        else
            print_success "SSH连接成功（预期结果）"
            return 0
        fi
    else
        if [[ "$expected_result" == "block" ]]; then
            print_success "SSH连接被阻断（预期结果）"
            return 0
        else
            print_error "SSH连接应该成功，但连接失败"
            return 1
        fi
    fi
}

# 检查HIPS日志
check_hips_logs() {
    print_info "检查HIPS日志..."
    
    if [ -f /proc/hips/logs ]; then
        echo "最近的HIPS日志:"
        cat /proc/hips/logs | tail -10
    else
        print_warning "无法访问HIPS日志"
    fi
}

# 检查HIPS统计
check_hips_stats() {
    print_info "检查HIPS统计信息..."
    
    if [ -f /proc/hips/status ]; then
        echo "HIPS统计信息:"
        cat /proc/hips/status
    else
        print_warning "无法访问HIPS状态"
    fi
}

# 清理测试规则
cleanup_test_rules() {
    print_info "清理测试规则..."
    
    # 这里需要实现清理规则的功能
    # 由于当前实现没有提供列出和删除特定规则的完整功能
    # 我们只能通过重新加载配置来清理
    if ./tools/hipsctl reload; then
        print_success "测试规则已清理"
    else
        print_warning "无法清理测试规则"
    fi
}

# 主测试函数
main_test() {
    echo "HIPS SSH阻断功能测试"
    echo "====================="
    echo ""
    
    # 基本检查
    check_root
    check_hips_module
    check_ssh_service
    
    # 获取测试IP
    local_ip=$(get_local_ip)
    test_ip="192.168.1.100"  # 测试IP
    
    print_info "本机IP: $local_ip"
    print_info "测试IP: $test_ip"
    echo ""
    
    # 测试1: 阻断特定IP的SSH连接
    print_info "测试1: 阻断特定IP的SSH连接"
    echo "----------------------------------------"
    
    # 添加阻断规则
    if add_ssh_block_rule "$test_ip" "阻断恶意IP的SSH连接"; then
        # 测试连接应该被阻断
        if test_ssh_connection "$test_ip" "block"; then
            print_success "测试1通过: SSH连接被正确阻断"
        else
            print_error "测试1失败: SSH连接未被阻断"
        fi
    else
        print_error "测试1失败: 无法添加阻断规则"
    fi
    
    echo ""
    
    # 测试2: 验证本机SSH连接正常
    print_info "测试2: 验证本机SSH连接正常"
    echo "----------------------------------------"
    
    # 测试本机连接应该成功
    if test_ssh_connection "$local_ip" "allow"; then
        print_success "测试2通过: 本机SSH连接正常"
    else
        print_error "测试2失败: 本机SSH连接异常"
    fi
    
    echo ""
    
    # 检查日志和统计
    check_hips_logs
    echo ""
    check_hips_stats
    echo ""
    
    # 清理测试
    cleanup_test_rules
    
    echo ""
    print_success "SSH阻断功能测试完成"
}

# 显示帮助信息
show_help() {
    echo "HIPS SSH阻断功能测试脚本"
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help     显示此帮助信息"
    echo "  -t, --test     运行完整测试"
    echo "  -c, --cleanup  清理测试规则"
    echo ""
    echo "示例:"
    echo "  sudo $0 --test"
    echo "  sudo $0 --cleanup"
}

# 主函数
main() {
    case "${1:-test}" in
        "test"|"-t"|"--test")
            main_test
            ;;
        "cleanup"|"-c"|"--cleanup")
            check_root
            cleanup_test_rules
            ;;
        "help"|"-h"|"--help")
            show_help
            ;;
        *)
            print_error "未知选项: $1"
            show_help
            exit 1
            ;;
    esac
}

# 运行主函数
main "$@" 
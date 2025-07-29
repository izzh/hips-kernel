#!/bin/bash

# HIPS Kernel Module - GitHub 推送脚本
# 使用方法: ./push-to-github.sh

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

# 检查Git状态
check_git_status() {
    print_info "检查Git状态..."
    
    if ! git status --porcelain | grep -q .; then
        print_success "工作目录干净"
    else
        print_warning "有未提交的更改"
        git status --short
    fi
}

# 检查远程仓库
check_remote() {
    print_info "检查远程仓库配置..."
    
    if git remote get-url origin > /dev/null 2>&1; then
        print_success "远程仓库已配置: $(git remote get-url origin)"
    else
        print_error "远程仓库未配置"
        exit 1
    fi
}

# 推送主分支
push_main() {
    print_info "推送主分支到GitHub..."
    
    if git push -u origin main; then
        print_success "主分支推送成功"
    else
        print_error "主分支推送失败"
        print_info "请检查网络连接和GitHub认证"
        return 1
    fi
}

# 推送标签
push_tags() {
    print_info "推送标签到GitHub..."
    
    if git push origin v1.0.0; then
        print_success "标签推送成功"
    else
        print_error "标签推送失败"
        return 1
    fi
}

# 显示GitHub仓库信息
show_repo_info() {
    print_info "GitHub仓库信息:"
    echo "  仓库URL: https://github.com/izzh/hips-kernel"
    echo "  克隆URL: https://github.com/izzh/hips-kernel.git"
    echo "  SSH URL: git@github.com:izzh/hips-kernel.git"
    echo ""
    print_info "如果推送失败，请尝试以下方法:"
    echo "1. 使用HTTPS + 个人访问令牌:"
    echo "   git remote set-url origin https://YOUR_TOKEN@github.com/izzh/hips-kernel.git"
    echo ""
    echo "2. 配置SSH密钥:"
    echo "   ssh-keygen -t ed25519 -C 'your_email@example.com'"
    echo "   ssh-add ~/.ssh/id_ed25519"
    echo "   # 将公钥添加到GitHub账户"
    echo ""
    echo "3. 手动推送:"
    echo "   git push -u origin main"
    echo "   git push origin v1.0.0"
}

# 主函数
main() {
    echo "HIPS Kernel Module - GitHub 推送脚本"
    echo "====================================="
    echo ""
    
    check_git_status
    check_remote
    
    echo ""
    print_info "开始推送到GitHub..."
    echo ""
    
    if push_main && push_tags; then
        echo ""
        print_success "所有内容已成功推送到GitHub!"
        echo ""
        show_repo_info
    else
        echo ""
        print_error "推送过程中遇到问题"
        echo ""
        show_repo_info
        exit 1
    fi
}

# 运行主函数
main "$@" 
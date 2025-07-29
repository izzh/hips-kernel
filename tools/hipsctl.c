#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>
#include "../include/hips.h"

#define HIPS_DEVICE "/dev/hips"

// 帮助信息
void print_help(void)
{
    printf("HIPS 控制工具 - 版本 %s\n", HIPS_MODULE_VERSION);
    printf("\n用法: hipsctl [选项] [命令]\n");
    printf("\n选项:\n");
    printf("  -h, --help     显示此帮助信息\n");
    printf("  -v, --version  显示版本信息\n");
    printf("  -d, --device   指定设备文件 (默认: %s)\n", HIPS_DEVICE);
    printf("\n命令:\n");
    printf("  status          显示模块状态\n");
    printf("  enable          启用模块\n");
    printf("  disable         禁用模块\n");
    printf("  reload          重新加载配置\n");
    printf("  stats           显示统计信息\n");
    printf("  logs            显示日志记录\n");
    printf("  add-rule        添加规则\n");
    printf("  del-rule        删除规则\n");
    printf("  list-rules      列出所有规则\n");
    printf("\n规则格式:\n");
    printf("  add-rule <类型> <动作> <优先级> <目标> [描述]\n");
    printf("  类型: exec|dns|network\n");
    printf("  动作: block|allow|log\n");
    printf("  优先级: 数字 (0-999)\n");
    printf("  目标: 文件路径|域名|IP地址\n");
    printf("\n示例:\n");
    printf("  hipsctl status\n");
    printf("  hipsctl add-rule exec block 100 /usr/bin/malware.exe 恶意软件\n");
    printf("  hipsctl add-rule dns block 50 evil.com 恶意域名\n");
    printf("  hipsctl add-rule network block 75 192.168.1.100 恶意IP\n");
}

// 版本信息
void print_version(void)
{
    printf("HIPS 控制工具 - 版本 %s\n", HIPS_MODULE_VERSION);
    printf("内核模块版本: %s\n", HIPS_MODULE_VERSION);
}

// 打开设备
int open_device(const char *device)
{
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "错误: 无法打开设备 %s: %s\n", device, strerror(errno));
        return -1;
    }
    return fd;
}

// 显示状态
int show_status(const char *device)
{
    int fd;
    struct hips_config config;
    struct hips_stats stats;
    
    fd = open_device(device);
    if (fd < 0) {
        return -1;
    }
    
    // 获取配置
    if (ioctl(fd, HIPS_IOCTL_GET_CONFIG, &config) == 0) {
        printf("HIPS 模块状态:\n");
        printf("  启用状态: %s\n", config.enabled ? "启用" : "禁用");
        printf("  日志级别: %u\n", config.log_level);
        printf("  最大规则数: %u\n", config.max_rules);
        printf("  配置文件: %s\n", config.config_file);
    } else {
        fprintf(stderr, "错误: 无法获取配置信息\n");
        close(fd);
        return -1;
    }
    
    // 获取统计信息
    if (ioctl(fd, HIPS_IOCTL_GET_STATS, &stats) == 0) {
        printf("\n统计信息:\n");
        printf("  执行阻止: %llu\n", stats.exec_blocks);
        printf("  DNS阻止: %llu\n", stats.dns_blocks);
        printf("  网络阻止: %llu\n", stats.network_blocks);
        printf("  总事件数: %llu\n", stats.total_events);
        printf("  最后事件: %llu\n", stats.last_event_time);
    } else {
        fprintf(stderr, "错误: 无法获取统计信息\n");
    }
    
    close(fd);
    return 0;
}

// 启用模块
int enable_module(const char *device)
{
    int fd;
    
    fd = open_device(device);
    if (fd < 0) {
        return -1;
    }
    
    if (ioctl(fd, HIPS_IOCTL_ENABLE) == 0) {
        printf("HIPS 模块已启用\n");
    } else {
        fprintf(stderr, "错误: 无法启用模块\n");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

// 禁用模块
int disable_module(const char *device)
{
    int fd;
    
    fd = open_device(device);
    if (fd < 0) {
        return -1;
    }
    
    if (ioctl(fd, HIPS_IOCTL_DISABLE) == 0) {
        printf("HIPS 模块已禁用\n");
    } else {
        fprintf(stderr, "错误: 无法禁用模块\n");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

// 重新加载配置
int reload_config(const char *device)
{
    int fd;
    
    fd = open_device(device);
    if (fd < 0) {
        return -1;
    }
    
    if (ioctl(fd, HIPS_IOCTL_RELOAD) == 0) {
        printf("配置已重新加载\n");
    } else {
        fprintf(stderr, "错误: 无法重新加载配置\n");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

// 显示统计信息
int show_stats(const char *device)
{
    int fd;
    struct hips_stats stats;
    
    fd = open_device(device);
    if (fd < 0) {
        return -1;
    }
    
    if (ioctl(fd, HIPS_IOCTL_GET_STATS, &stats) == 0) {
        printf("HIPS 统计信息:\n");
        printf("  执行阻止: %llu\n", stats.exec_blocks);
        printf("  DNS阻止: %llu\n", stats.dns_blocks);
        printf("  网络阻止: %llu\n", stats.network_blocks);
        printf("  总事件数: %llu\n", stats.total_events);
        printf("  最后事件: %llu\n", stats.last_event_time);
    } else {
        fprintf(stderr, "错误: 无法获取统计信息\n");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

// 显示日志
int show_logs(const char *device)
{
    int fd;
    struct hips_log_entry entry;
    int count = 0;
    
    fd = open_device(device);
    if (fd < 0) {
        return -1;
    }
    
    printf("HIPS 日志记录:\n");
    printf("========================================\n");
    
    while (ioctl(fd, HIPS_IOCTL_GET_LOGS, &entry) == 0 && count < 100) {
        printf("[%llu] 规则ID: %u, 类型: %u, 动作: %u\n",
               entry.timestamp, entry.rule_id, entry.rule_type, entry.action);
        printf("  进程: %s (PID: %u)\n", entry.process_name, entry.pid);
        printf("  目标: %s\n", entry.target);
        printf("  详情: %s\n", entry.details);
        printf("----------------------------------------\n");
        count++;
    }
    
    if (count == 0) {
        printf("暂无日志记录\n");
    }
    
    close(fd);
    return 0;
}

// 添加规则
int add_rule(const char *device, int argc, char *argv[])
{
    int fd;
    struct hips_rule rule;
    
    if (argc < 5) {
        fprintf(stderr, "错误: 参数不足\n");
        fprintf(stderr, "用法: add-rule <类型> <动作> <优先级> <目标> [描述]\n");
        return -1;
    }
    
    // 解析规则类型
    if (strcmp(argv[0], "exec") == 0) {
        rule.rule_type = HIPS_RULE_EXEC;
    } else if (strcmp(argv[0], "dns") == 0) {
        rule.rule_type = HIPS_RULE_DNS;
    } else if (strcmp(argv[0], "network") == 0) {
        rule.rule_type = HIPS_RULE_NETWORK;
    } else {
        fprintf(stderr, "错误: 无效的规则类型: %s\n", argv[0]);
        return -1;
    }
    
    // 解析动作
    if (strcmp(argv[1], "block") == 0) {
        rule.action = HIPS_ACTION_BLOCK;
    } else if (strcmp(argv[1], "allow") == 0) {
        rule.action = HIPS_ACTION_ALLOW;
    } else if (strcmp(argv[1], "log") == 0) {
        rule.action = HIPS_ACTION_LOG;
    } else {
        fprintf(stderr, "错误: 无效的动作: %s\n", argv[1]);
        return -1;
    }
    
    // 解析优先级
    if (sscanf(argv[2], "%u", &rule.priority) != 1) {
        fprintf(stderr, "错误: 无效的优先级: %s\n", argv[2]);
        return -1;
    }
    
    // 设置目标
    strncpy(rule.target, argv[3], sizeof(rule.target) - 1);
    rule.target[sizeof(rule.target) - 1] = '\0';
    
    // 设置描述
    if (argc > 4) {
        strncpy(rule.description, argv[4], sizeof(rule.description) - 1);
        rule.description[sizeof(rule.description) - 1] = '\0';
    } else {
        strcpy(rule.description, "");
    }
    
    // 设置规则ID为0，让内核自动分配
    rule.rule_id = 0;
    
    fd = open_device(device);
    if (fd < 0) {
        return -1;
    }
    
    if (ioctl(fd, HIPS_IOCTL_ADD_RULE, &rule) == 0) {
        printf("规则添加成功，ID: %u\n", rule.rule_id);
    } else {
        fprintf(stderr, "错误: 无法添加规则\n");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

// 删除规则
int del_rule(const char *device, int argc, char *argv[])
{
    int fd;
    u32 rule_id;
    
    if (argc < 1) {
        fprintf(stderr, "错误: 请指定规则ID\n");
        return -1;
    }
    
    if (sscanf(argv[0], "%u", &rule_id) != 1) {
        fprintf(stderr, "错误: 无效的规则ID: %s\n", argv[0]);
        return -1;
    }
    
    fd = open_device(device);
    if (fd < 0) {
        return -1;
    }
    
    if (ioctl(fd, HIPS_IOCTL_DEL_RULE, rule_id) == 0) {
        printf("规则删除成功: ID=%u\n", rule_id);
    } else {
        fprintf(stderr, "错误: 无法删除规则\n");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

// 列出规则
int list_rules(const char *device)
{
    // 这里实现列出规则的逻辑
    // 由于内核模块没有提供列出所有规则的IOCTL，这里简化实现
    printf("规则列表功能暂未实现\n");
    return 0;
}

int main(int argc, char *argv[])
{
    const char *device = HIPS_DEVICE;
    const char *command = NULL;
    int opt;
    
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"device", required_argument, 0, 'd'},
        {0, 0, 0, 0}
    };
    
    // 解析命令行选项
    while ((opt = getopt_long(argc, argv, "hvd:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_help();
                return 0;
            case 'v':
                print_version();
                return 0;
            case 'd':
                device = optarg;
                break;
            default:
                print_help();
                return 1;
        }
    }
    
    // 检查是否有命令
    if (optind >= argc) {
        fprintf(stderr, "错误: 请指定命令\n");
        print_help();
        return 1;
    }
    
    command = argv[optind];
    
    // 执行命令
    if (strcmp(command, "status") == 0) {
        return show_status(device);
    } else if (strcmp(command, "enable") == 0) {
        return enable_module(device);
    } else if (strcmp(command, "disable") == 0) {
        return disable_module(device);
    } else if (strcmp(command, "reload") == 0) {
        return reload_config(device);
    } else if (strcmp(command, "stats") == 0) {
        return show_stats(device);
    } else if (strcmp(command, "logs") == 0) {
        return show_logs(device);
    } else if (strcmp(command, "add-rule") == 0) {
        return add_rule(device, argc - optind - 1, &argv[optind + 1]);
    } else if (strcmp(command, "del-rule") == 0) {
        return del_rule(device, argc - optind - 1, &argv[optind + 1]);
    } else if (strcmp(command, "list-rules") == 0) {
        return list_rules(device);
    } else {
        fprintf(stderr, "错误: 未知命令: %s\n", command);
        print_help();
        return 1;
    }
} 
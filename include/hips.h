#ifndef HIPS_H
#define HIPS_H

#include <linux/types.h>
#include <linux/ioctl.h>

// 模块信息
#define HIPS_MODULE_NAME "hips"
#define HIPS_MODULE_VERSION "1.0.0"

// 规则类型
#define HIPS_RULE_EXEC     1
#define HIPS_RULE_DNS      2
#define HIPS_RULE_NETWORK  3

// 动作类型
#define HIPS_ACTION_BLOCK  0
#define HIPS_ACTION_ALLOW  1
#define HIPS_ACTION_LOG    2

// 规则结构体
struct hips_rule {
    __u32 rule_id;
    __u32 rule_type;
    __u32 action;
    __u32 priority;
    char target[256];
    char description[512];
};

// 配置结构体
struct hips_config {
    __u32 enabled;
    __u32 log_level;
    __u32 max_rules;
    char config_file[256];
};

// 统计信息结构体
struct hips_stats {
    __u64 exec_blocks;
    __u64 dns_blocks;
    __u64 network_blocks;
    __u64 total_events;
    __u64 last_event_time;
};

// 日志条目结构体
struct hips_log_entry {
    __u64 timestamp;
    __u32 rule_id;
    __u32 rule_type;
    __u32 action;
    __u32 pid;
    char process_name[256];
    char target[256];
    char details[512];
};

// IOCTL 命令
#define HIPS_MAGIC 'H'

#define HIPS_IOCTL_ADD_RULE     _IOW(HIPS_MAGIC, 1, struct hips_rule)
#define HIPS_IOCTL_DEL_RULE     _IOW(HIPS_MAGIC, 2, __u32)
#define HIPS_IOCTL_GET_RULE     _IOWR(HIPS_MAGIC, 3, struct hips_rule)
#define HIPS_IOCTL_SET_CONFIG   _IOW(HIPS_MAGIC, 4, struct hips_config)
#define HIPS_IOCTL_GET_CONFIG   _IOR(HIPS_MAGIC, 5, struct hips_config)
#define HIPS_IOCTL_GET_STATS    _IOR(HIPS_MAGIC, 6, struct hips_stats)
#define HIPS_IOCTL_GET_LOGS     _IOWR(HIPS_MAGIC, 7, struct hips_log_entry)
#define HIPS_IOCTL_ENABLE       _IO(HIPS_MAGIC, 8)
#define HIPS_IOCTL_DISABLE      _IO(HIPS_MAGIC, 9)
#define HIPS_IOCTL_RELOAD       _IO(HIPS_MAGIC, 10)

// 错误码
#define HIPS_SUCCESS            0
#define HIPS_ERROR_INVALID      -1
#define HIPS_ERROR_NOT_FOUND    -2
#define HIPS_ERROR_EXISTS       -3
#define HIPS_ERROR_PERMISSION   -4
#define HIPS_ERROR_MEMORY       -5

// 日志级别
#define HIPS_LOG_ERROR          0
#define HIPS_LOG_WARN           1
#define HIPS_LOG_INFO           2
#define HIPS_LOG_DEBUG          3

// 网络地址结构体
struct hips_network_addr {
    __u32 family;  // AF_INET or AF_INET6
    union {
        __u32 ipv4;
        __u8 ipv6[16];
    } addr;
    __u16 port;
};

// 进程信息结构体
struct hips_process_info {
    __u32 pid;
    __u32 ppid;
    __u32 uid;
    __u32 gid;
    char comm[256];
    char exe[256];
};

#endif /* HIPS_H */ 
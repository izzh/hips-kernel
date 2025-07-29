#ifndef HIPS_COMMON_H
#define HIPS_COMMON_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/rwlock.h>
#include <linux/security.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/version.h>
#include <linux/cred.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/nsproxy.h>
#include <linux/ns_common.h>
#include <linux/user_namespace.h>

#include "../include/hips.h"

// 模块参数
static int hips_enabled = 1;
static int hips_log_level = HIPS_LOG_INFO;
static int hips_max_rules = 1000;
static char hips_config_file[256] = "/etc/hips/config.json";

module_param(hips_enabled, int, 0644);
module_param(hips_log_level, int, 0644);
module_param(hips_max_rules, int, 0644);
module_param_string(hips_config_file, hips_config_file, sizeof(hips_config_file), 0644);

// 规则结构体
struct hips_rule_entry {
    struct list_head list;
    struct hips_rule rule;
    spinlock_t lock;
    atomic_t ref_count;
};

// 全局配置结构体
struct hips_global_config {
    spinlock_t config_lock;
    struct list_head exec_rules;
    struct list_head dns_rules;
    struct list_head network_rules;
    struct hips_stats stats;
    struct hips_config config;
    struct proc_dir_entry *proc_dir;
    struct cdev *cdev;
    dev_t dev_num;
};

// 日志条目结构体
struct hips_log_entry_internal {
    struct list_head list;
    struct hips_log_entry entry;
    unsigned long timestamp;
};

// 全局变量
extern struct hips_global_config *hips_config;

// 函数声明
// 主模块函数
int hips_init_module(void);
void hips_exit_module(void);

// 钩子函数
int hips_exec_hook(struct linux_binprm *bprm);
int hips_dns_hook(struct sk_buff *skb, const struct nf_hook_state *state);
int hips_network_hook(struct sk_buff *skb, const struct nf_hook_state *state);

// 规则管理函数
int hips_add_rule(struct hips_rule *rule);
int hips_del_rule(u32 rule_id);
int hips_get_rule(u32 rule_id, struct hips_rule *rule);
int hips_match_rule(u32 rule_type, const char *target, struct hips_rule *matched_rule);

// 配置管理函数
int hips_load_config(void);
int hips_save_config(void);
int hips_reload_config(void);

// 日志函数
void hips_log_event(u32 rule_id, u32 rule_type, u32 action, u32 pid, 
                   const char *process_name, const char *target, const char *details);
int hips_get_logs(struct hips_log_entry *entries, int max_entries);

// 统计函数
void hips_update_stats(u32 rule_type, u32 action);
int hips_get_stats(struct hips_stats *stats);

// 用户空间接口函数
int hips_open(struct inode *inode, struct file *file);
int hips_release(struct inode *inode, struct file *file);
long hips_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
ssize_t hips_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
ssize_t hips_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);

// 工具函数
char *hips_get_process_name(struct task_struct *task);
int hips_parse_ip(const char *ip_str, struct hips_network_addr *addr);
int hips_match_ip(const struct hips_network_addr *addr1, const struct hips_network_addr *addr2);
int hips_match_pattern(const char *pattern, const char *string);

// 兼容性宏定义
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
    #define HIPS_KERNEL_5_PLUS
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
    #define HIPS_KERNEL_6_PLUS
#endif

// 调试宏
#ifdef CONFIG_HIPS_DEBUG
    #define HIPS_DEBUG(fmt, ...) \
        printk(KERN_DEBUG "HIPS: " fmt "\n", ##__VA_ARGS__)
#else
    #define HIPS_DEBUG(fmt, ...) do {} while(0)
#endif

#define HIPS_INFO(fmt, ...) \
    printk(KERN_INFO "HIPS: " fmt "\n", ##__VA_ARGS__)

#define HIPS_WARN(fmt, ...) \
    printk(KERN_WARNING "HIPS: " fmt "\n", ##__VA_ARGS__)

#define HIPS_ERROR(fmt, ...) \
    printk(KERN_ERR "HIPS: " fmt "\n", ##__VA_ARGS__)

#endif /* HIPS_COMMON_H */ 
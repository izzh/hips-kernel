#include "hips_common.h"

// 文件操作结构体
static const struct file_operations hips_status_fops = {
    .owner = THIS_MODULE,
    .read = hips_status_read,
    .llseek = seq_lseek,
    .release = single_release,
    .open = hips_status_open,
};

static const struct file_operations hips_rules_fops = {
    .owner = THIS_MODULE,
    .read = hips_rules_read,
    .write = hips_rules_write,
    .llseek = seq_lseek,
    .release = single_release,
    .open = hips_rules_open,
};

static const struct file_operations hips_logs_fops = {
    .owner = THIS_MODULE,
    .read = hips_logs_read,
    .llseek = seq_lseek,
    .release = single_release,
    .open = hips_logs_open,
};

// 状态文件操作
static int hips_status_show(struct seq_file *m, void *v)
{
    struct hips_stats stats;
    
    if (!hips_config) {
        seq_printf(m, "HIPS 模块未加载\n");
        return 0;
    }
    
    // 获取统计信息
    if (hips_get_stats(&stats) == 0) {
        seq_printf(m, "HIPS 状态信息:\n");
        seq_printf(m, "  模块状态: %s\n", 
                  hips_config->config.enabled ? "启用" : "禁用");
        seq_printf(m, "  日志级别: %u\n", hips_config->config.log_level);
        seq_printf(m, "  最大规则数: %u\n", hips_config->config.max_rules);
        seq_printf(m, "  配置文件: %s\n", hips_config->config.config_file);
        seq_printf(m, "\n统计信息:\n");
        seq_printf(m, "  执行阻止: %llu\n", stats.exec_blocks);
        seq_printf(m, "  DNS阻止: %llu\n", stats.dns_blocks);
        seq_printf(m, "  网络阻止: %llu\n", stats.network_blocks);
        seq_printf(m, "  总事件数: %llu\n", stats.total_events);
        seq_printf(m, "  最后事件: %llu\n", stats.last_event_time);
    } else {
        seq_printf(m, "无法获取统计信息\n");
    }
    
    return 0;
}

static int hips_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, hips_status_show, NULL);
}

static ssize_t hips_status_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    return seq_read(file, buf, count, ppos);
}

// 规则文件操作
static int hips_rules_show(struct seq_file *m, void *v)
{
    struct hips_rule_entry *entry;
    struct list_head *rule_lists[] = {
        &hips_config->exec_rules,
        &hips_config->dns_rules,
        &hips_config->network_rules
    };
    const char *rule_types[] = {"执行", "DNS", "网络"};
    int i;
    
    if (!hips_config) {
        seq_printf(m, "HIPS 模块未加载\n");
        return 0;
    }
    
    seq_printf(m, "HIPS 规则列表:\n");
    seq_printf(m, "========================================\n");
    
    for (i = 0; i < ARRAY_SIZE(rule_lists); i++) {
        seq_printf(m, "\n%s 规则:\n", rule_types[i]);
        seq_printf(m, "----------------------------------------\n");
        
        spin_lock(&hips_config->config_lock);
        list_for_each_entry(entry, rule_lists[i], list) {
            seq_printf(m, "ID: %u\n", entry->rule.rule_id);
            seq_printf(m, "类型: %s\n", rule_types[entry->rule.rule_type - 1]);
            seq_printf(m, "动作: %s\n", 
                      entry->rule.action == HIPS_ACTION_BLOCK ? "阻止" :
                      entry->rule.action == HIPS_ACTION_ALLOW ? "允许" : "记录");
            seq_printf(m, "优先级: %u\n", entry->rule.priority);
            seq_printf(m, "目标: %s\n", entry->rule.target);
            seq_printf(m, "描述: %s\n", entry->rule.description);
            seq_printf(m, "----------------------------------------\n");
        }
        spin_unlock(&hips_config->config_lock);
    }
    
    return 0;
}

static int hips_rules_open(struct inode *inode, struct file *file)
{
    return single_open(file, hips_rules_show, NULL);
}

static ssize_t hips_rules_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    return seq_read(file, buf, count, ppos);
}

static ssize_t hips_rules_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char *data;
    int ret;
    
    if (count > 4096) {
        return -EINVAL;
    }
    
    data = kmalloc(count + 1, GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }
    
    if (copy_from_user(data, buf, count)) {
        kfree(data);
        return -EFAULT;
    }
    data[count] = '\0';
    
    // 解析命令
    ret = hips_parse_rules_command(data, count);
    
    kfree(data);
    return ret < 0 ? ret : count;
}

// 日志文件操作
static int hips_logs_show(struct seq_file *m, void *v)
{
    struct hips_log_entry entries[100];
    int count, i;
    
    if (!hips_config) {
        seq_printf(m, "HIPS 模块未加载\n");
        return 0;
    }
    
    seq_printf(m, "HIPS 日志记录:\n");
    seq_printf(m, "========================================\n");
    
    count = hips_get_logs(entries, ARRAY_SIZE(entries));
    if (count > 0) {
        for (i = 0; i < count; i++) {
            seq_printf(m, "[%llu] 规则ID: %u, 类型: %u, 动作: %u\n",
                      entries[i].timestamp, entries[i].rule_id,
                      entries[i].rule_type, entries[i].action);
            seq_printf(m, "  进程: %s (PID: %u)\n", 
                      entries[i].process_name, entries[i].pid);
            seq_printf(m, "  目标: %s\n", entries[i].target);
            seq_printf(m, "  详情: %s\n", entries[i].details);
            seq_printf(m, "----------------------------------------\n");
        }
    } else {
        seq_printf(m, "暂无日志记录\n");
    }
    
    return 0;
}

static int hips_logs_open(struct inode *inode, struct file *file)
{
    return single_open(file, hips_logs_show, NULL);
}

static ssize_t hips_logs_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    return seq_read(file, buf, count, ppos);
}

// 解析规则命令
int hips_parse_rules_command(const char *data, size_t size)
{
    char *line, *saveptr;
    char *data_copy;
    int ret = 0;
    
    data_copy = kmalloc(size + 1, GFP_KERNEL);
    if (!data_copy) {
        return -ENOMEM;
    }
    
    memcpy(data_copy, data, size);
    data_copy[size] = '\0';
    
    // 逐行解析
    line = strtok_r(data_copy, "\n", &saveptr);
    while (line) {
        ret = hips_parse_rule_line(line);
        if (ret < 0) {
            HIPS_ERROR("解析规则行失败: %s", line);
            break;
        }
        line = strtok_r(NULL, "\n", &saveptr);
    }
    
    kfree(data_copy);
    return ret;
}

// 解析规则行
int hips_parse_rule_line(const char *line)
{
    struct hips_rule rule;
    char *token, *saveptr;
    char *line_copy;
    int ret = 0;
    
    line_copy = kstrdup(line, GFP_KERNEL);
    if (!line_copy) {
        return -ENOMEM;
    }
    
    // 跳过空白字符
    while (*line_copy == ' ' || *line_copy == '\t') {
        line_copy++;
    }
    
    // 跳过注释行
    if (*line_copy == '#' || *line_copy == '\0') {
        kfree(line_copy);
        return 0;
    }
    
    // 解析规则格式: 类型|动作|优先级|目标|描述
    token = strtok_r(line_copy, "|", &saveptr);
    if (!token) {
        ret = -EINVAL;
        goto out;
    }
    
    // 解析规则类型
    if (strcmp(token, "exec") == 0) {
        rule.rule_type = HIPS_RULE_EXEC;
    } else if (strcmp(token, "dns") == 0) {
        rule.rule_type = HIPS_RULE_DNS;
    } else if (strcmp(token, "network") == 0) {
        rule.rule_type = HIPS_RULE_NETWORK;
    } else {
        ret = -EINVAL;
        goto out;
    }
    
    // 解析动作
    token = strtok_r(NULL, "|", &saveptr);
    if (!token) {
        ret = -EINVAL;
        goto out;
    }
    
    if (strcmp(token, "block") == 0) {
        rule.action = HIPS_ACTION_BLOCK;
    } else if (strcmp(token, "allow") == 0) {
        rule.action = HIPS_ACTION_ALLOW;
    } else if (strcmp(token, "log") == 0) {
        rule.action = HIPS_ACTION_LOG;
    } else {
        ret = -EINVAL;
        goto out;
    }
    
    // 解析优先级
    token = strtok_r(NULL, "|", &saveptr);
    if (!token) {
        ret = -EINVAL;
        goto out;
    }
    
    if (kstrtou32(token, 10, &rule.priority) != 0) {
        ret = -EINVAL;
        goto out;
    }
    
    // 解析目标
    token = strtok_r(NULL, "|", &saveptr);
    if (!token) {
        ret = -EINVAL;
        goto out;
    }
    
    strncpy(rule.target, token, sizeof(rule.target) - 1);
    rule.target[sizeof(rule.target) - 1] = '\0';
    
    // 解析描述
    token = strtok_r(NULL, "|", &saveptr);
    if (token) {
        strncpy(rule.description, token, sizeof(rule.description) - 1);
        rule.description[sizeof(rule.description) - 1] = '\0';
    } else {
        strcpy(rule.description, "");
    }
    
    // 添加规则
    rule.rule_id = 0; // 自动分配ID
    ret = hips_add_rule(&rule);
    
out:
    kfree(line_copy);
    return ret;
} 
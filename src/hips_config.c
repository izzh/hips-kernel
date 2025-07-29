#include "hips_common.h"

// 规则计数器
static atomic_t rule_id_counter = ATOMIC_INIT(0);

// 添加规则
int hips_add_rule(struct hips_rule *rule)
{
    struct hips_rule_entry *entry;
    struct list_head *rule_list;
    
    if (!hips_config || !rule) {
        return HIPS_ERROR_INVALID;
    }
    
    // 分配规则条目
    entry = kzalloc(sizeof(struct hips_rule_entry), GFP_KERNEL);
    if (!entry) {
        HIPS_ERROR("无法分配规则内存");
        return HIPS_ERROR_MEMORY;
    }
    
    // 设置规则ID
    if (rule->rule_id == 0) {
        rule->rule_id = atomic_inc_return(&rule_id_counter);
    }
    
    // 复制规则数据
    memcpy(&entry->rule, rule, sizeof(struct hips_rule));
    
    // 初始化锁和引用计数
    spin_lock_init(&entry->lock);
    atomic_set(&entry->ref_count, 1);
    
    // 根据规则类型选择列表
    switch (rule->rule_type) {
        case HIPS_RULE_EXEC:
            rule_list = &hips_config->exec_rules;
            break;
        case HIPS_RULE_DNS:
            rule_list = &hips_config->dns_rules;
            break;
        case HIPS_RULE_NETWORK:
            rule_list = &hips_config->network_rules;
            break;
        default:
            HIPS_ERROR("无效的规则类型: %u", rule->rule_type);
            kfree(entry);
            return HIPS_ERROR_INVALID;
    }
    
    // 添加到规则列表
    spin_lock(&hips_config->config_lock);
    list_add_tail(&entry->list, rule_list);
    spin_unlock(&hips_config->config_lock);
    
    HIPS_INFO("添加规则成功: ID=%u, 类型=%u, 目标=%s", 
              rule->rule_id, rule->rule_type, rule->target);
    
    return HIPS_SUCCESS;
}

// 删除规则
int hips_del_rule(u32 rule_id)
{
    struct hips_rule_entry *entry, *tmp;
    struct list_head *rule_lists[] = {
        &hips_config->exec_rules,
        &hips_config->dns_rules,
        &hips_config->network_rules
    };
    int i;
    
    if (!hips_config) {
        return HIPS_ERROR_INVALID;
    }
    
    spin_lock(&hips_config->config_lock);
    
    // 在所有规则列表中查找并删除
    for (i = 0; i < ARRAY_SIZE(rule_lists); i++) {
        list_for_each_entry_safe(entry, tmp, rule_lists[i], list) {
            if (entry->rule.rule_id == rule_id) {
                list_del(&entry->list);
                spin_unlock(&hips_config->config_lock);
                
                // 等待引用计数归零
                while (atomic_read(&entry->ref_count) > 1) {
                    schedule();
                }
                
                kfree(entry);
                HIPS_INFO("删除规则成功: ID=%u", rule_id);
                return HIPS_SUCCESS;
            }
        }
    }
    
    spin_unlock(&hips_config->config_lock);
    HIPS_WARN("未找到规则: ID=%u", rule_id);
    return HIPS_ERROR_NOT_FOUND;
}

// 获取规则
int hips_get_rule(u32 rule_id, struct hips_rule *rule)
{
    struct hips_rule_entry *entry;
    struct list_head *rule_lists[] = {
        &hips_config->exec_rules,
        &hips_config->dns_rules,
        &hips_config->network_rules
    };
    int i;
    
    if (!hips_config || !rule) {
        return HIPS_ERROR_INVALID;
    }
    
    spin_lock(&hips_config->config_lock);
    
    // 在所有规则列表中查找
    for (i = 0; i < ARRAY_SIZE(rule_lists); i++) {
        list_for_each_entry(entry, rule_lists[i], list) {
            if (entry->rule.rule_id == rule_id) {
                memcpy(rule, &entry->rule, sizeof(struct hips_rule));
                spin_unlock(&hips_config->config_lock);
                return HIPS_SUCCESS;
            }
        }
    }
    
    spin_unlock(&hips_config->config_lock);
    return HIPS_ERROR_NOT_FOUND;
}

// 匹配规则
int hips_match_rule(u32 rule_type, const char *target, struct hips_rule *matched_rule)
{
    struct hips_rule_entry *entry;
    struct list_head *rule_list;
    
    if (!hips_config || !target || !matched_rule) {
        return HIPS_ERROR_INVALID;
    }
    
    // 根据规则类型选择列表
    switch (rule_type) {
        case HIPS_RULE_EXEC:
            rule_list = &hips_config->exec_rules;
            break;
        case HIPS_RULE_DNS:
            rule_list = &hips_config->dns_rules;
            break;
        case HIPS_RULE_NETWORK:
            rule_list = &hips_config->network_rules;
            break;
        default:
            return HIPS_ERROR_INVALID;
    }
    
    spin_lock(&hips_config->config_lock);
    
    // 遍历规则列表进行匹配
    list_for_each_entry(entry, rule_list, list) {
        if (entry->rule.rule_type == rule_type) {
            if (hips_match_pattern(entry->rule.target, target)) {
                atomic_inc(&entry->ref_count);
                memcpy(matched_rule, &entry->rule, sizeof(struct hips_rule));
                spin_unlock(&hips_config->config_lock);
                return HIPS_SUCCESS;
            }
        }
    }
    
    spin_unlock(&hips_config->config_lock);
    return HIPS_ERROR_NOT_FOUND;
}

// 模式匹配（支持通配符）
int hips_match_pattern(const char *pattern, const char *string)
{
    if (!pattern || !string) {
        return 0;
    }
    
    // 精确匹配
    if (strcmp(pattern, string) == 0) {
        return 1;
    }
    
    // 通配符匹配（简化实现）
    if (strstr(pattern, "*") || strstr(pattern, "?")) {
        // 这里实现通配符匹配逻辑
        // 简化实现，实际需要更复杂的模式匹配
        return 0;
    }
    
    // 前缀匹配
    if (pattern[strlen(pattern) - 1] == '*') {
        char prefix[256];
        strncpy(prefix, pattern, strlen(pattern) - 1);
        prefix[strlen(pattern) - 1] = '\0';
        if (strncmp(string, prefix, strlen(prefix)) == 0) {
            return 1;
        }
    }
    
    return 0;
}

// 加载配置
int hips_load_config(void)
{
    struct file *file;
    char *buf;
    loff_t pos = 0;
    int ret = 0;
    
    if (!hips_config) {
        return HIPS_ERROR_INVALID;
    }
    
    // 打开配置文件
    file = filp_open(hips_config->config.config_file, O_RDONLY, 0);
    if (IS_ERR(file)) {
        HIPS_WARN("无法打开配置文件: %s", hips_config->config.config_file);
        return HIPS_ERROR_INVALID;
    }
    
    // 读取文件内容
    buf = kmalloc(file->f_inode->i_size + 1, GFP_KERNEL);
    if (!buf) {
        filp_close(file, NULL);
        return HIPS_ERROR_MEMORY;
    }
    
    ret = kernel_read(file, buf, file->f_inode->i_size, &pos);
    if (ret < 0) {
        HIPS_ERROR("读取配置文件失败: %d", ret);
        kfree(buf);
        filp_close(file, NULL);
        return ret;
    }
    
    buf[ret] = '\0';
    
    // 解析配置文件（简化实现）
    ret = hips_parse_config(buf, ret);
    
    kfree(buf);
    filp_close(file, NULL);
    
    if (ret == 0) {
        HIPS_INFO("配置文件加载成功");
    }
    
    return ret;
}

// 保存配置
int hips_save_config(void)
{
    struct file *file;
    char *buf;
    loff_t pos = 0;
    int ret = 0;
    
    if (!hips_config) {
        return HIPS_ERROR_INVALID;
    }
    
    // 生成配置内容
    buf = hips_generate_config();
    if (!buf) {
        return HIPS_ERROR_MEMORY;
    }
    
    // 创建配置文件
    file = filp_open(hips_config->config.config_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (IS_ERR(file)) {
        HIPS_ERROR("无法创建配置文件: %s", hips_config->config.config_file);
        kfree(buf);
        return HIPS_ERROR_INVALID;
    }
    
    // 写入配置内容
    ret = kernel_write(file, buf, strlen(buf), &pos);
    if (ret < 0) {
        HIPS_ERROR("写入配置文件失败: %d", ret);
    } else {
        HIPS_INFO("配置文件保存成功");
    }
    
    kfree(buf);
    filp_close(file, NULL);
    
    return ret;
}

// 重新加载配置
int hips_reload_config(void)
{
    int ret;
    
    if (!hips_config) {
        return HIPS_ERROR_INVALID;
    }
    
    HIPS_INFO("重新加载配置");
    
    // 清理现有规则
    hips_cleanup_rules();
    
    // 加载新配置
    ret = hips_load_config();
    
    return ret;
}

// 清理规则列表
void hips_cleanup_rules(void)
{
    struct hips_rule_entry *entry, *tmp;
    struct list_head *rule_lists[] = {
        &hips_config->exec_rules,
        &hips_config->dns_rules,
        &hips_config->network_rules
    };
    int i;
    
    if (!hips_config) {
        return;
    }
    
    spin_lock(&hips_config->config_lock);
    
    for (i = 0; i < ARRAY_SIZE(rule_lists); i++) {
        list_for_each_entry_safe(entry, tmp, rule_lists[i], list) {
            list_del(&entry->list);
            kfree(entry);
        }
    }
    
    spin_unlock(&hips_config->config_lock);
    
    HIPS_INFO("规则列表清理完成");
}

// 解析配置文件（简化实现）
int hips_parse_config(const char *config_data, size_t size)
{
    // 这里实现配置文件解析
    // 简化实现，实际需要解析 JSON 或其他格式
    HIPS_INFO("解析配置文件，大小: %zu", size);
    return 0;
}

// 生成配置文件（简化实现）
char *hips_generate_config(void)
{
    char *buf;
    size_t size = 1024;
    
    buf = kmalloc(size, GFP_KERNEL);
    if (!buf) {
        return NULL;
    }
    
    snprintf(buf, size, 
             "{\n"
             "  \"enabled\": %u,\n"
             "  \"log_level\": %u,\n"
             "  \"max_rules\": %u\n"
             "}\n",
             hips_config->config.enabled,
             hips_config->config.log_level,
             hips_config->config.max_rules);
    
    return buf;
} 
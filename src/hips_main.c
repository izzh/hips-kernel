#include "hips_common.h"

// 全局配置
struct hips_global_config *hips_config = NULL;

// 文件操作结构体
static const struct file_operations hips_fops = {
    .owner = THIS_MODULE,
    .open = hips_open,
    .release = hips_release,
    .read = hips_read,
    .write = hips_write,
    .unlocked_ioctl = hips_ioctl,
    .llseek = noop_llseek,
};

// 模块初始化函数
static int __init hips_init(void)
{
    int ret = 0;
    
    HIPS_INFO("HIPS 内核模块初始化开始");
    
    // 分配全局配置结构体
    hips_config = kzalloc(sizeof(struct hips_global_config), GFP_KERNEL);
    if (!hips_config) {
        HIPS_ERROR("无法分配全局配置内存");
        return -ENOMEM;
    }
    
    // 初始化配置
    spin_lock_init(&hips_config->config_lock);
    INIT_LIST_HEAD(&hips_config->exec_rules);
    INIT_LIST_HEAD(&hips_config->dns_rules);
    INIT_LIST_HEAD(&hips_config->network_rules);
    
    // 初始化统计信息
    memset(&hips_config->stats, 0, sizeof(struct hips_stats));
    hips_config->stats.last_event_time = ktime_get_ns();
    
    // 初始化配置
    hips_config->config.enabled = hips_enabled;
    hips_config->config.log_level = hips_log_level;
    hips_config->config.max_rules = hips_max_rules;
    strncpy(hips_config->config.config_file, hips_config_file, sizeof(hips_config->config.config_file) - 1);
    
    // 注册字符设备
    ret = alloc_chrdev_region(&hips_config->dev_num, 0, 1, HIPS_MODULE_NAME);
    if (ret < 0) {
        HIPS_ERROR("无法分配设备号: %d", ret);
        goto error_alloc_dev;
    }
    
    hips_config->cdev = cdev_alloc();
    if (!hips_config->cdev) {
        HIPS_ERROR("无法分配字符设备");
        ret = -ENOMEM;
        goto error_alloc_cdev;
    }
    
    hips_config->cdev->ops = &hips_fops;
    hips_config->cdev->owner = THIS_MODULE;
    
    ret = cdev_add(hips_config->cdev, hips_config->dev_num, 1);
    if (ret < 0) {
        HIPS_ERROR("无法添加字符设备: %d", ret);
        goto error_add_cdev;
    }
    
    // 创建 /proc 接口
    hips_config->proc_dir = proc_mkdir("hips", NULL);
    if (!hips_config->proc_dir) {
        HIPS_ERROR("无法创建 /proc/hips 目录");
        ret = -ENOMEM;
        goto error_proc_dir;
    }
    
    // 创建 /proc/hips/status
    if (!proc_create("status", 0444, hips_config->proc_dir, NULL)) {
        HIPS_ERROR("无法创建 /proc/hips/status");
        ret = -ENOMEM;
        goto error_proc_status;
    }
    
    // 创建 /proc/hips/rules
    if (!proc_create("rules", 0644, hips_config->proc_dir, NULL)) {
        HIPS_ERROR("无法创建 /proc/hips/rules");
        ret = -ENOMEM;
        goto error_proc_rules;
    }
    
    // 创建 /proc/hips/logs
    if (!proc_create("logs", 0444, hips_config->proc_dir, NULL)) {
        HIPS_ERROR("无法创建 /proc/hips/logs");
        ret = -ENOMEM;
        goto error_proc_logs;
    }
    
    // 注册安全钩子
    ret = hips_register_hooks();
    if (ret < 0) {
        HIPS_ERROR("无法注册安全钩子: %d", ret);
        goto error_hooks;
    }
    
    // 加载配置
    ret = hips_load_config();
    if (ret < 0) {
        HIPS_WARN("无法加载配置文件，使用默认配置");
    }
    
    HIPS_INFO("HIPS 内核模块初始化完成");
    return 0;
    
error_hooks:
    remove_proc_entry("logs", hips_config->proc_dir);
error_proc_logs:
    remove_proc_entry("rules", hips_config->proc_dir);
error_proc_rules:
    remove_proc_entry("status", hips_config->proc_dir);
error_proc_status:
    proc_remove(hips_config->proc_dir);
error_proc_dir:
    cdev_del(hips_config->cdev);
error_add_cdev:
    kfree(hips_config->cdev);
error_alloc_cdev:
    unregister_chrdev_region(hips_config->dev_num, 1);
error_alloc_dev:
    kfree(hips_config);
    hips_config = NULL;
    
    return ret;
}

// 模块退出函数
static void __exit hips_exit(void)
{
    HIPS_INFO("HIPS 内核模块卸载开始");
    
    if (!hips_config) {
        return;
    }
    
    // 注销安全钩子
    hips_unregister_hooks();
    
    // 保存配置
    hips_save_config();
    
    // 清理规则列表
    hips_cleanup_rules();
    
    // 移除 /proc 接口
    if (hips_config->proc_dir) {
        remove_proc_entry("logs", hips_config->proc_dir);
        remove_proc_entry("rules", hips_config->proc_dir);
        remove_proc_entry("status", hips_config->proc_dir);
        proc_remove(hips_config->proc_dir);
    }
    
    // 注销字符设备
    if (hips_config->cdev) {
        cdev_del(hips_config->cdev);
        kfree(hips_config->cdev);
    }
    
    if (hips_config->dev_num) {
        unregister_chrdev_region(hips_config->dev_num, 1);
    }
    
    // 释放全局配置
    kfree(hips_config);
    hips_config = NULL;
    
    HIPS_INFO("HIPS 内核模块卸载完成");
}

// 模块信息
MODULE_LICENSE("GPL");
MODULE_AUTHOR("HIPS Development Team");
MODULE_DESCRIPTION("Host Intrusion Prevention System Kernel Module");
MODULE_VERSION(HIPS_MODULE_VERSION);

// 模块参数描述
module_param_named(enabled, hips_enabled, int, 0644);
MODULE_PARM_DESC(enabled, "启用/禁用 HIPS 模块 (1/0)");

module_param_named(log_level, hips_log_level, int, 0644);
MODULE_PARM_DESC(log_level, "日志级别 (0=ERROR, 1=WARN, 2=INFO, 3=DEBUG)");

module_param_named(max_rules, hips_max_rules, int, 0644);
MODULE_PARM_DESC(max_rules, "最大规则数量");

module_param_named(config_file, hips_config_file, charp, 0644);
MODULE_PARM_DESC(config_file, "配置文件路径");

// 模块初始化和退出宏
module_init(hips_init);
module_exit(hips_exit); 
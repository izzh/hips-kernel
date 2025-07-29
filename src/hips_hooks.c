#include "hips_common.h"

// 安全钩子结构体
static struct security_hook_list hips_hooks[] = {
    LSM_HOOK_INIT(bprm_check, hips_exec_hook),
};

// Netfilter 钩子结构体
static struct nf_hook_ops hips_nf_ops[] = {
    {
        .hook = hips_dns_hook,
        .pf = NFPROTO_IPV4,
        .hooknum = NF_INET_PRE_ROUTING,
        .priority = NF_IP_PRI_FIRST,
    },
    {
        .hook = hips_network_hook,
        .pf = NFPROTO_IPV4,
        .hooknum = NF_INET_LOCAL_OUT,
        .priority = NF_IP_PRI_FIRST,
    },
#ifdef CONFIG_IPV6
    {
        .hook = hips_dns_hook,
        .pf = NFPROTO_IPV6,
        .hooknum = NF_INET_PRE_ROUTING,
        .priority = NF_IP_PRI_FIRST,
    },
    {
        .hook = hips_network_hook,
        .pf = NFPROTO_IPV6,
        .hooknum = NF_INET_LOCAL_OUT,
        .priority = NF_IP_PRI_FIRST,
    },
#endif
};

// 注册安全钩子
int hips_register_hooks(void)
{
    int ret;
    
    // 注册 LSM 钩子
    ret = security_add_hooks(hips_hooks, ARRAY_SIZE(hips_hooks), "hips");
    if (ret < 0) {
        HIPS_ERROR("无法注册 LSM 钩子: %d", ret);
        return ret;
    }
    
    // 注册 Netfilter 钩子
    ret = nf_register_net_hooks(&init_net, hips_nf_ops, ARRAY_SIZE(hips_nf_ops));
    if (ret < 0) {
        HIPS_ERROR("无法注册 Netfilter 钩子: %d", ret);
        security_delete_hooks(hips_hooks, ARRAY_SIZE(hips_hooks));
        return ret;
    }
    
    HIPS_INFO("安全钩子注册成功");
    return 0;
}

// 注销安全钩子
void hips_unregister_hooks(void)
{
    nf_unregister_net_hooks(&init_net, hips_nf_ops, ARRAY_SIZE(hips_nf_ops));
    security_delete_hooks(hips_hooks, ARRAY_SIZE(hips_hooks));
    HIPS_INFO("安全钩子注销完成");
}

// 进程执行钩子
int hips_exec_hook(struct linux_binprm *bprm)
{
    struct hips_rule matched_rule;
    char *process_name;
    char *exe_path;
    int ret = 0;
    
    if (!hips_config || !hips_config->config.enabled) {
        return 0;
    }
    
    // 获取进程信息
    process_name = hips_get_process_name(current);
    exe_path = bprm->filename;
    
    HIPS_DEBUG("进程执行检查: %s (%s)", process_name, exe_path);
    
    // 检查执行规则
    if (hips_match_rule(HIPS_RULE_EXEC, exe_path, &matched_rule) == 0) {
        if (matched_rule.action == HIPS_ACTION_BLOCK) {
            HIPS_WARN("阻止进程执行: %s (规则ID: %u)", exe_path, matched_rule.rule_id);
            
            // 记录事件
            hips_log_event(matched_rule.rule_id, HIPS_RULE_EXEC, HIPS_ACTION_BLOCK,
                          current->pid, process_name, exe_path, "进程执行被阻止");
            
            // 更新统计
            hips_update_stats(HIPS_RULE_EXEC, HIPS_ACTION_BLOCK);
            
            return -EPERM;
        } else if (matched_rule.action == HIPS_ACTION_LOG) {
            HIPS_INFO("记录进程执行: %s (规则ID: %u)", exe_path, matched_rule.rule_id);
            hips_log_event(matched_rule.rule_id, HIPS_RULE_EXEC, HIPS_ACTION_LOG,
                          current->pid, process_name, exe_path, "进程执行被记录");
        }
    }
    
    return ret;
}

// DNS 查询钩子
int hips_dns_hook(struct sk_buff *skb, const struct nf_hook_state *state)
{
    struct hips_rule matched_rule;
    struct udphdr *udp;
    struct tcphdr *tcp;
    char domain[256];
    int ret = NF_ACCEPT;
    
    if (!hips_config || !hips_config->config.enabled) {
        return NF_ACCEPT;
    }
    
    // 检查是否是 DNS 查询
    if (state->pf == NFPROTO_IPV4) {
        struct iphdr *iph = ip_hdr(skb);
        if (iph->protocol == IPPROTO_UDP) {
            udp = udp_hdr(skb);
            if (ntohs(udp->dest) != 53) {
                return NF_ACCEPT;
            }
        } else if (iph->protocol == IPPROTO_TCP) {
            tcp = tcp_hdr(skb);
            if (ntohs(tcp->dest) != 53) {
                return NF_ACCEPT;
            }
        } else {
            return NF_ACCEPT;
        }
    }
#ifdef CONFIG_IPV6
    else if (state->pf == NFPROTO_IPV6) {
        struct ipv6hdr *ip6h = ipv6_hdr(skb);
        if (ip6h->nexthdr == IPPROTO_UDP) {
            udp = udp_hdr(skb);
            if (ntohs(udp->dest) != 53) {
                return NF_ACCEPT;
            }
        } else if (ip6h->nexthdr == IPPROTO_TCP) {
            tcp = tcp_hdr(skb);
            if (ntohs(tcp->dest) != 53) {
                return NF_ACCEPT;
            }
        } else {
            return NF_ACCEPT;
        }
    }
#endif
    else {
        return NF_ACCEPT;
    }
    
    // 解析域名（简化实现）
    if (hips_parse_dns_query(skb, domain, sizeof(domain)) == 0) {
        HIPS_DEBUG("DNS 查询检查: %s", domain);
        
        // 检查 DNS 规则
        if (hips_match_rule(HIPS_RULE_DNS, domain, &matched_rule) == 0) {
            if (matched_rule.action == HIPS_ACTION_BLOCK) {
                HIPS_WARN("阻止 DNS 查询: %s (规则ID: %u)", domain, matched_rule.rule_id);
                
                // 记录事件
                hips_log_event(matched_rule.rule_id, HIPS_RULE_DNS, HIPS_ACTION_BLOCK,
                              current->pid, "dns", domain, "DNS 查询被阻止");
                
                // 更新统计
                hips_update_stats(HIPS_RULE_DNS, HIPS_ACTION_BLOCK);
                
                return NF_DROP;
            } else if (matched_rule.action == HIPS_ACTION_LOG) {
                HIPS_INFO("记录 DNS 查询: %s (规则ID: %u)", domain, matched_rule.rule_id);
                hips_log_event(matched_rule.rule_id, HIPS_RULE_DNS, HIPS_ACTION_LOG,
                              current->pid, "dns", domain, "DNS 查询被记录");
            }
        }
    }
    
    return ret;
}

// 网络连接钩子
int hips_network_hook(struct sk_buff *skb, const struct nf_hook_state *state)
{
    struct hips_rule matched_rule;
    struct hips_network_addr addr;
    char addr_str[64];
    int ret = NF_ACCEPT;
    
    if (!hips_config || !hips_config->config.enabled) {
        return NF_ACCEPT;
    }
    
    // 解析网络地址
    if (hips_parse_network_addr(skb, &addr) == 0) {
        hips_format_network_addr(&addr, addr_str, sizeof(addr_str));
        HIPS_DEBUG("网络连接检查: %s", addr_str);
        
        // 检查网络规则
        if (hips_match_rule(HIPS_RULE_NETWORK, addr_str, &matched_rule) == 0) {
            if (matched_rule.action == HIPS_ACTION_BLOCK) {
                HIPS_WARN("阻止网络连接: %s (规则ID: %u)", addr_str, matched_rule.rule_id);
                
                // 记录事件
                hips_log_event(matched_rule.rule_id, HIPS_RULE_NETWORK, HIPS_ACTION_BLOCK,
                              current->pid, "network", addr_str, "网络连接被阻止");
                
                // 更新统计
                hips_update_stats(HIPS_RULE_NETWORK, HIPS_ACTION_BLOCK);
                
                return NF_DROP;
            } else if (matched_rule.action == HIPS_ACTION_LOG) {
                HIPS_INFO("记录网络连接: %s (规则ID: %u)", addr_str, matched_rule.rule_id);
                hips_log_event(matched_rule.rule_id, HIPS_RULE_NETWORK, HIPS_ACTION_LOG,
                              current->pid, "network", addr_str, "网络连接被记录");
            }
        }
    }
    
    return ret;
}

// 解析 DNS 查询（简化实现）
int hips_parse_dns_query(struct sk_buff *skb, char *domain, size_t domain_size)
{
    // 这里实现 DNS 查询解析
    // 简化实现，实际需要解析 DNS 包结构
    memset(domain, 0, domain_size);
    strncpy(domain, "example.com", domain_size - 1);
    return 0;
}

// 解析网络地址
int hips_parse_network_addr(struct sk_buff *skb, struct hips_network_addr *addr)
{
    if (skb->protocol == htons(ETH_P_IP)) {
        struct iphdr *iph = ip_hdr(skb);
        addr->family = AF_INET;
        addr->addr.ipv4 = iph->daddr;
        addr->port = 0; // 需要从 TCP/UDP 头获取
        return 0;
    }
#ifdef CONFIG_IPV6
    else if (skb->protocol == htons(ETH_P_IPV6)) {
        struct ipv6hdr *ip6h = ipv6_hdr(skb);
        addr->family = AF_INET6;
        memcpy(addr->addr.ipv6, &ip6h->daddr, 16);
        addr->port = 0; // 需要从 TCP/UDP 头获取
        return 0;
    }
#endif
    
    return -1;
}

// 格式化网络地址
void hips_format_network_addr(struct hips_network_addr *addr, char *str, size_t size)
{
    if (addr->family == AF_INET) {
        snprintf(str, size, "%pI4:%d", &addr->addr.ipv4, addr->port);
    }
#ifdef CONFIG_IPV6
    else if (addr->family == AF_INET6) {
        snprintf(str, size, "%pI6:%d", &addr->addr.ipv6, addr->port);
    }
#endif
    else {
        snprintf(str, size, "unknown");
    }
} 
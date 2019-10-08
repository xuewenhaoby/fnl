#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter_ipv4.h>
#include <net/ip.h>
#include <net/net_namespace.h>

static struct nf_hook_ops i_hook;
static struct iphdr *ip_header;
static struct tcphdr *tcp_header;
static struct net *net;

static void update_pktype(struct sk_buff *skb, struct iphdr *iph, struct tcphdr *tcp)
{
	skb->pkt_type = PACKET_HOST;
}


static unsigned int _hook_incoming(void* priv, struct sk_buff* skb, const struct nf_hook_state* st)
{
	skb_linearize(skb);
	ip_header = ip_hdr(skb);
	tcp_header = tcp_hdr(skb);

	if(ip_header->protocol == IPPROTO_TCP)
	{
		// printk("<tcpmod> tcp pkt\n");
		// printk("<tcpmod> saddr: %lx, sport: %x, daddr: %lx, dport: %x\n", ip_header->saddr, tcp_header->source, ip_header->daddr, tcp_header->dest);
		if(ntohs(tcp_header->dest) == 9999 || ntohs(tcp_header->source) == 9999)
		{
			// printk("<tcpmod> match port 9999\n");
			update_pktype(skb, ip_header, tcp_header);
			return NF_ACCEPT;
		}
	}
	return NF_ACCEPT;
}

static void install_hooks(void)
{
	i_hook.hook = _hook_incoming;
	i_hook.hooknum = NF_INET_PRE_ROUTING;
	i_hook.pf = PF_INET;
	i_hook.priority = NF_IP_PRI_FIRST;
	nf_register_hook(&i_hook);
}

static void uninstall_hooks(void)
{
    nf_unregister_hook(&i_hook);
}

static int __init tcpmod_init(void)
{
	install_hooks();
	return 0;
}

static void __exit tcpmod_exit(void)
{
	uninstall_hooks();
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("change pkt type");
MODULE_AUTHOR("sjm");

module_init(tcpmod_init);
module_exit(tcpmod_exit);

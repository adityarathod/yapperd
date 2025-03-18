#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

// Cursor to iterate over packet headers
struct hdr_cursor
{
    void *pos;
    void *end;
};

/**
 * Parse Ethernet header and return the next header type
 */
static __always_inline int parse_ethhdr(struct hdr_cursor *nh, struct ethhdr **eth_hdr)
{
    struct ethhdr *eth = nh->pos;
    int hdr_size = sizeof(*eth);

    // eBPF required bounds check: if the current position + size of the header is greater than the
    // end of the packet, we return -1 to indicate an error.
    // This is equivalent to writing `if (nh->pos + hdr_size > nh->end) { return -1; }`
    if (eth + 1 > (struct ethhdr *)nh->end)
    {
        return -1;
    }

    // Move the cursor to the next header
    nh->pos += hdr_size;
    // Set eth_hdr to the current Ethernet header
    *eth_hdr = eth;

    // Return the next header type
    return eth->h_proto;
}

/**
 * Parse IPv4 header and return the L4 protocol type
 */
static __always_inline int parse_iphdr(struct hdr_cursor *nh, struct iphdr **ip_hdr)
{
    struct iphdr *ip = nh->pos;
    int hdr_size;

    // eBPF required bounds check: if the current position + size of the header is greater than the
    // end of the packet, we return -1 to indicate an error.
    // This is equivalent to writing `if (nh->pos + hdr_size > nh->end) { return -1; }`
    if (ip + 1 > (struct iphdr *)nh->end)
    {
        return -1;
    }

    // Calculate the header size based on the IHL field
    hdr_size = ip->ihl * 4;
    if (nh->pos + hdr_size > nh->end)
    {
        return -1;
    }

    // Move the cursor to the next header
    nh->pos += hdr_size;
    // Set ip_hdr to the current IP header
    *ip_hdr = ip;

    // Return the protocol type
    return ip->protocol;
}

SEC("xdp")
int xdp_drop_ipv6_non_tcp(struct xdp_md *ctx)
{
    // Maintain packet pointers
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;
    struct hdr_cursor nh = {.pos = data, .end = data_end};

    // Header pointers
    struct ethhdr *eth;
    struct iphdr *ip;

    // Header types
    int nh_type;
    int ip_type;

    // Parse Ethernet header and pass only IPv4 packets
    nh_type = parse_ethhdr(&nh, &eth);
    if (nh_type != bpf_htons(ETH_P_IP))
    {
        return XDP_DROP;
    }

    // Parse IPv4 header to get the L4 protocol type and pass only TCP packets
    ip_type = parse_iphdr(&nh, &ip);
    if (ip_type != IPPROTO_TCP)
    {
        return XDP_DROP;
    }

    return XDP_PASS;
}

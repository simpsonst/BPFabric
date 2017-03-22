// Example based on Brendan Gregg's code available at http://www.brendangregg.com/blog/2015-05-15/ebpf-one-small-step.html

// #include <linux/if_ether.h>
// #include <linux/ip.h>
// #include <linux/icmp.h>
#include "ebpf_consts.h"
#include "ebpf_functions.h"
#include "ebpf_switch.h"

struct bpf_map_def SEC("maps") traffichist = {
    .type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(uint32_t),
    .value_size = sizeof(uint64_t),
    .max_entries = 24,
};

struct bpf_map_def SEC("maps") inports = {
    .type = BPF_MAP_TYPE_HASH,
    .key_size = 6, // MAC address is the key
    .value_size = sizeof(uint32_t),
    .max_entries = 256,
};

uint64_t prog(struct packet *pkt)
{
    // Packet distribution
    uint32_t index = pkt->metadata.length / 64;
    uint64_t *value;

    bpf_map_lookup_elem(&traffichist, &index, &value);
    (*value)++;

    // Learning Switch
    uint32_t *out_port;

    // if the source is not a broadcast or multicast
    if ((pkt->eth.h_source[0] & 1) == 0) {
        // Update the port associated with the packet
        bpf_map_update_elem(&inports, pkt->eth.h_source, &pkt->metadata.in_port, 0);
    }

    // Flood of the destination is broadcast or multicast
    if (pkt->eth.h_dest[0] & 1) {
        return FLOOD;
    }

    // Lookup the output port
    if (bpf_map_lookup_elem(&inports, pkt->eth.h_dest, &out_port) == -1) {
        // If no entry was found flood
        return FLOOD;
    }

    return *out_port;
}
char _license[] SEC("license") = "GPL";

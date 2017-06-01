#include <linux/if_ether.h>
#include "ebpf_consts.h"
#include "ebpf_functions.h"
#include "ebpf_switch.h"

struct bpf_map_def SEC("maps") inports = {
    .type = BPF_MAP_TYPE_HASH,
    .key_size = 6, // MAC address is the key
    .value_size = sizeof(uint32_t),
    .max_entries = 10000,
};

uint64_t prog(struct packet *pkt)
{
    uint32_t flood = FLOOD;
    uint32_t *out_port;

    // if the source is not a broadcast or multicast
    if ((pkt->eth.h_source[0] & 1) == 0) {
        // Update the port associated with the packet
        bpf_map_update_elem(&inports, pkt->eth.h_source, &pkt->metadata.in_port, 0);
    }

    // Flood if the destination is broadcast or multicast
    if (pkt->eth.h_dest[0] & 1) {
        out_port = &flood;
    } else if (/* Lookup the output port */
               bpf_map_lookup_elem(&inports, pkt->eth.h_dest, &out_port) == -1) {
        // If no entry was found flood
        out_port = &flood;
    }

    //bpf_trace("TRANSFER", pkt->metadata.in_port, *out_port);
    return *out_port;
}
char _license[] SEC("license") = "GPL";
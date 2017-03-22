// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

#ifndef __EBPF_SWITCH_H
#define __EBPF_SWITCH_H

#if __BPF__
#define SEC(NAME) __attribute__((section(NAME), used))
#else
#define SEC(NAME)
#endif

enum bpf_map_type {
    BPF_MAP_TYPE_UNSPEC,
    BPF_MAP_TYPE_HASH,
    BPF_MAP_TYPE_ARRAY,
};

struct bpf_map_def {
    unsigned int type;
    unsigned int key_size;
    unsigned int value_size;
    unsigned int max_entries;
    unsigned int map_flags;
};

#define BPF_MAP(I,T,K,V,N,F)             \
    struct bpf_map_def SEC("maps") I = { \
        .map_type = BPF_MAP_TYPE_##T,    \
        .key_size = sizeof(K),           \
        .value_size = sizeof(V),         \
        .max_entries = (N),              \
        .map_flags = (F),                \
    }

#endif

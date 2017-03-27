// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

#ifndef __EBPF_HASHTAB_H
#define __EBPF_HASHTAB_H

#include "bpfmap.h"

struct bpf_map *htab_map_alloc(union bpf_attr *attr);
void *htab_map_lookup_elem(struct bpf_map *map, const void *key);
int htab_map_update_elem(struct bpf_map *map, const void *key,
                         const void *value, uint64_t map_flags);
int htab_map_delete_elem(struct bpf_map *map, const void *key);
void htab_map_free(struct bpf_map *map);
int htab_map_get_next_key(struct bpf_map *map, const void *key, void *next_key);

#endif

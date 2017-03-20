// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

#ifndef __EBPF_SWITCH_FUNCTIONS_H
#define __EBPF_SWITCH_FUNCTIONS_H

#include "ebpf_fnhelp.h"
#include "ebpf_fntypes.h"

BPF_FNDEFN(bpf_map_lookup_elem);
BPF_FNDEFN(bpf_map_update_elem);
BPF_FNDEFN(bpf_map_delete_elem);
BPF_FNDEFN(bpf_notify);
BPF_FNDEFN(bpf_debug);

#endif

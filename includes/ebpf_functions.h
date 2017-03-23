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
BPF_FNDEFN(bzero);
BPF_FNDEFN(bsalt);
BPF_FNDEFN(bcopy);
BPF_FNDEFN(digest_init);
BPF_FNDEFN(digest_update);
BPF_FNDEFN(digest_final);
BPF_FNDEFN(crypt_verify);

#endif

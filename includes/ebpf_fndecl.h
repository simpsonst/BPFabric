// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

#ifndef __EBPF_SWITCH_FNDECL_H
#define __EBPF_SWITCH_FNDECL_H

#include "ebpf_fnhelp.h"
#include "ebpf_fntypes.h"
                     
BPF_FN(bpf_map_lookup_elem);
BPF_FN(bpf_map_update_elem);
BPF_FN(bpf_map_delete_elem);
BPF_FN(bpf_notify);
BPF_FN(bpf_debug);
BPF_FN(bzero);
BPF_FN(bsalt);
BPF_FN(bcopy);
BPF_FN(digest_init);
BPF_FN(digest_update);
BPF_FN(digest_final);
BPF_FN(crypt_verify);

#endif

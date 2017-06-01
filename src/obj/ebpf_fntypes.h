// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

#ifndef __EBPF_SWITCH_FNTYPES_H
#define __EBPF_SWITCH_FNTYPES_H

#ifdef __cplusplus
extern "C" {
#endif

    /* TODO: Find a header that robustly defines size_t in both native
       and eBPF environments. */
#if __BPF__ && (__clang_major__ < 3 || \
                (__clang_major__ == 3 && __clang_minor__ <= 9))
    typedef unsigned long long size_t;
#else
    /* clang-3.9 might not be able to find this.  What can be used for
       defining size_t? */
#include <stddef.h>
#endif

#include "ebpf_fnhelp.h"

    /* Each external function must have its own function type.  From
       native C, it should be declared using this type to ensure
       matching signatures. */
    typedef int BPF_FNTYPE(bpf_map_lookup_elem)(void *map,
                                                const void *key, void *value);
    typedef int BPF_FNTYPE(bpf_map_update_elem)(void *map, const void *key,
                                                const void *value,
                                                unsigned long long flags);
    typedef int BPF_FNTYPE(bpf_map_delete_elem)(void *map, const void *key);
    typedef void *BPF_FNTYPE(bpf_notify)(int id, void *data, int len);
    typedef void *BPF_FNTYPE(bpf_debug)(const char *);
    typedef void BPF_FNTYPE(bpf_trace)(const char *, uint64_t ln, uint64_t v);
    typedef void BPF_FNTYPE(bzero)(void *, size_t);
    typedef void BPF_FNTYPE(bsalt)(void *, size_t);
    typedef void BPF_FNTYPE(bcopy)(const void *src, void *dest, size_t n);
    typedef void BPF_FNTYPE(digest_init)(void *ctxt, const void *params);
    typedef void BPF_FNTYPE(digest_update)(void *ctxt, const void *, size_t);
    typedef void BPF_FNTYPE(digest_final)(void *ctxt, void *res);
    typedef int BPF_FNTYPE(crypt_verify)(void *ctxt,
                                         const unsigned char *hash,
                                         size_t hashlen,
                                         const unsigned char *sig,
                                         const unsigned char *pubkey);

    /* Each external function needs a unique numeric code. */
    enum {
        BPF_FUNC_bpf_map_lookup_elem = 1,
        BPF_FUNC_bpf_map_update_elem = 2,
        BPF_FUNC_bpf_map_delete_elem = 3,
        BPF_FUNC_bpf_trace = 23,
        BPF_FUNC_bsalt = 24,
        BPF_FUNC_crypt_verify = 25,
        BPF_FUNC_digest_init = 26,
        BPF_FUNC_digest_update = 27,
        BPF_FUNC_digest_final = 28,
        BPF_FUNC_bcopy = 29,
        BPF_FUNC_bzero = 30,
        BPF_FUNC_bpf_notify = 31,
        BPF_FUNC_bpf_debug = 32,
    };

    /* Additional functions should also be defined in
       <ebpf_functions.h> and <ebpf_fnimpl.h>. */

#ifdef __cplusplus
}
#endif

#endif
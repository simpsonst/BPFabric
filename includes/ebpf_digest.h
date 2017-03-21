// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

#ifndef __EBPF_SWITCH_DIGEST_H
#define __EBPF_SWITCH_DIGEST_H

#include <openssl/sha.h>

#ifdef __cplusplus
extern "C" {
#endif

    enum digest {
        digest_SHA256,
    };

#define DIGEST_CTXT(DT) digest_ctxt_ ## DT ## _t = { .type = digest_ ## DT }
#define DIGEST_SIZE(DT) DIGEST_SIZE_ ## DT
#define DIGEST(DT) digest_ ## DT ## _t

#define DECLARE_DIGEST_TYPE(DT)                         \
    typedef unsigned char DIGEST(DT)[DIGEST_SIZE(DT)]

    typedef struct {
        enum digest type;
    } digest_ctxt_t;

#define DIGEST_SIZE_SHA256 SHA256_DIGEST_LENGTH

    typedef struct {
        enum digest type;
        SHA256_CTX state;
    } digest_ctxt_SHA256_t;

    DECLARE_DIGEST_TYPE(SHA256);

#ifdef __cplusplus
}
#endif

#endif

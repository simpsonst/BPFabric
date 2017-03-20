// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

#ifndef __EBPF_SWITCH_DIGEST_H
#define __EBPF_SWITCH_DIGEST_H

#include <openssl/sha.h>

enum digest {
    digest_SHA256,
};

#define DIGEST_CTXT(DT) digest_ctxt_ ## DT = { .type = digest_ ## DT }
#define DIGEST_SIZE(DT) DIGEST_SIZE_ ## DT
#define DIGEST(DT) digest_ ## DT ## _t

#define DECLARE_DIGEST_TYPE(DT) \
    typedef unsigned char DIGEST(DT)[DIGEST_SIZE(DT)]

struct digest_ctxt {
    enum digest type;
};

#define DIGEST_SIZE_SHA256 SHA256_DIGEST_LENGTH

struct digest_ctxt_SHA256 {
    enum digest type;
    SHA256_CTX state;
};

DECLARE_DIGEST_TYPE(SHA256);

#endif

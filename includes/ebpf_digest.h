// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

#ifndef __EBPF_SWITCH_DIGEST_H
#define __EBPF_SWITCH_DIGEST_H

enum digest {
    digest_SHA256,
};

#define DIGEST_CTXT(DT) digest_ctxt_ ## DT = { .type = digest_ ## DT }
#define DIGEST_SIZE(DT) DIGEST_SIZE_ ## DT
#define DIGEST(DT) digest_ ## DT ## _t

struct digest_ctxt {
    enum digest type;
};

struct digest_ctxt_SHA256 {
    enum digest type;
    /* TODO */
};

#endif

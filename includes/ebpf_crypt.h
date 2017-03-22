// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

#ifndef __EBPF_SWITCH_CRYPT_H
#define __EBPF_SWITCH_CRYPT_H

#include <uECC.h>

#ifdef __cplusplus
extern "C" {
#endif

    enum crypt {
        crypt_SEPC256K1,
    };

#define CRYPT_CTXT1(CT, V) crypt_ctxt_ ## CT ## _t V = { .type = crypt_ ## CT }
#define CRYPT_CTXT(CT, V) CRYPT_CTXT1(CT, V)
#define CRYPT_SIGSIZE1(CT) CRYPT_SIGSIZE_ ## CT
#define CRYPT_SIGSIZE(CT) CRYPT_SIGSIZE1(CT)
#define CRYPT_SIG1(CT) crypt_ ## CT ## _sig_t
#define CRYPT_SIG(CT) CRYPT_SIG1(CT)
#define CRYPT_PUBKEYSIZE1(CT) CRYPT_PUBKEYSIZE_ ## CT
#define CRYPT_PUBKEYSIZE(CT) CRYPT_PUBKEYSIZE1(CT)
#define CRYPT_PUBKEY1(CT) crypt_ ## CT ## _pubkey_t
#define CRYPT_PUBKEY(CT) CRYPT_PUBKEY1(CT)

#define DECLARE_CRYPT_SIGTYPE(CT)                               \
    typedef unsigned char CRYPT_SIG(CT)[CRYPT_SIGSIZE(CT)]
#define DECLARE_CRYPT_PUBKEYTYPE(CT)                                    \
    typedef unsigned char CRYPT_PUBKEY(CT)[CRYPT_PUBKEYSIZE(CT)]

    typedef struct {
        enum crypt type;
    } crypt_ctxt_t;


#define CRYPT_SIGSIZE_SEPC256K1 64
#define CRYPT_PUBKEYSIZE_SEPC256K1 64

    typedef crypt_ctxt_t crypt_ctxt_SEPC256K1_t;

    DECLARE_CRYPT_SIGTYPE(SEPC256K1);
    DECLARE_CRYPT_PUBKEYTYPE(SEPC256K1);

#ifdef __cplusplus
}
#endif

#endif

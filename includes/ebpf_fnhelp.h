// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

#ifndef __EBPF_SWITCH_FNHELP_H
#define __EBPF_SWITCH_FNHELP_H

#define BPF_FNTYPE(N) bpf_ ## N ## _ft

#define BPF_FNAS(N, A) static BPF_FNTYPE(N) *const A
#define BPF_FN(N) BPF_FNAS(N, N)

#define BPF_FNDEFNAS(N, A) BPF_FNAS(N, A) = (BPF_FNTYPE(N) *) BPF_FUNC_ ## N
#define BPF_FNDEFN(N) BPF_FNDEFNAS(N, N)

#if ! __BPF__
/* Use these from native code to check signatures. */
#define BPF_FNCHECKAS(N, A) extern BPF_FNTYPE(N) A
#define BPF_FNCHECK(N) BPF_FNCHECKAS(N, N)

/* Expose a function to the VM. */
#define BPF_REGISTERAS(VM, N, A) ubpf_register((VM), BPF_FUNC_ ## N, #N, (A))
#define BPF_REGISTER(VM, N) BPF_REGISTERAS((VM), N, N)
#endif

#endif
